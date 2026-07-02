#include "app_controller.h"
#include "app_config.h"
#include "app_delay.h"
#include "app_uart.h"
#include "ch8_measure.h"
#include "gpio.h"
#include "hy65_ascii_cmd.h"
#include "usart.h"
#include "wireless_board.h"
#include "zdyh_cmd.h"

#define APP_REMOTE_WAKE_TIMEOUT_MS       10000U
#define APP_ZIGBEE_REINIT_INTERVAL_MS    5000U
#define APP_IDLE_SLEEP_TIMEOUT_MS        1800000U
#define APP_REMOTE_READY_BLINK_COUNT     3U
#define APP_REMOTE_READY_BLINK_MS        165U
#define APP_LAST_MSG_SIZE                20U

extern void SystemClock_Config(void);

static u8 TxBuffer2[MB_BUF_SIZE];
static u8 RxBuffer2[MB_BUF_SIZE];

static int TestValue[CH8_TEST_VALUE_SIZE] = {0};

typedef enum
{
	APP_STATE_FIRST_RUN = 0,
	APP_STATE_JUDGE_REMOTE_MSG,
	APP_STATE_SYS_INIT,
	APP_STATE_RUN_MODE
} APP_STATE;

typedef struct
{
	APP_STATE state;
	u8 remoteModuleOk;
	u8 measureFlag;
	u8 relayMode;
	u8 errorCode;
	u8 bcState;
	u8 bcResp;
	u32 lastBroadcastId;
	uint8_t lastCmd;
	uint8_t lastMsg[APP_LAST_MSG_SIZE];
	u32 wakeTick;
	u32 zigbeeRefreshTick;
	u32 idleSleepTick;
} APP_CONTEXT;

/**
  * @brief  Blink the status LED to indicate that the remote module is available.
  * @retval None
  */
static void App_BlinkRemoteReady(void)
{
	u8 i = 0;

	for(i = 0;i < APP_REMOTE_READY_BLINK_COUNT;i++)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
		delay_ms(APP_REMOTE_READY_BLINK_MS);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
		delay_ms(APP_REMOTE_READY_BLINK_MS);
	}
}

/**
  * @brief  Power and verify wireless modules during boot.
  * @retval 1 if the remote module is available, otherwise 0.
  */
static u8 App_CheckWirelessAtStartup(void)
{
	u8 remoteModuleOk = 0;

	MX_GPIO_Init();
	Wireless_PowerOnStartupModules();
	Wireless_InitZigbeeOrFault();
	remoteModuleOk = Wireless_CheckRemoteModule();
	Wireless_PowerOffModules();

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
	HAL_Delay(10);

	return remoteModuleOk;
}

/**
  * @brief  Restore clocks and LPUART receive after Stop-mode wake-up.
  * @retval None
  */
static void App_RestoreAfterStopWake(void)
{
	HAL_PWREx_DisableUltraLowPower();
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
	HAL_Init();
	SystemClock_Config();
	HAL_NVIC_DisableIRQ(EXTI4_15_IRQn);
	delayMicroseconds(WIRELESS_STOP_WAKE_SETTLE_US);

	Wireless_M0M1ModeConfig();
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
	HAL_Delay(10);

	MX_LPUART1_UART_Init();
	HAL_UART_Receive_IT(&hlpuart1, mb_usart3_t.rx_buf, MB_BUF_SIZE);
}

/**
  * @brief  Enter Stop mode and arm LPUART receive for remote wake data.
  * @param[in,out] ctx Application state context.
  * @retval None
  */
static void App_EnterStopAndWaitRemote(APP_CONTEXT *ctx)
{
	App_BlinkRemoteReady();
	Wireless_PrepareStopMode();
	Wireless_RestoreAfterStopMode();
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

	App_RestoreAfterStopWake();
	uwTick = 0;
	ctx->wakeTick = 0;
}

/**
  * @brief  Check whether a 433 MHz remote frame asks the board to start.
  * @param[in] rxBuffer Received remote-frame buffer.
  * @param[in] length Received frame length.
  * @retval 1 if at least one channel command is active, otherwise 0.
  */
static u8 App_RemoteCommandRequestsStart(const u8 *rxBuffer, int length)
{
	int colonIndex = -1;
	int i = 0;

	if(length <= 0)
	{
		return 0;
	}

	for(i = 0;i < length && i < 4;i++)
	{
		if(rxBuffer[i] == ':')
		{
			colonIndex = i;
			break;
		}
	}
	if(colonIndex < 0 || length < colonIndex + 7)
	{
		return 0;
	}
	if(rxBuffer[colonIndex + 5] != 0x0D || rxBuffer[colonIndex + 6] != 0x0A)
	{
		return 0;
	}

	for(i = 1;i < 5;i++)
	{
		if(rxBuffer[colonIndex + i] > '0')
		{
			return 1;
		}
	}
	return 0;
}

/**
  * @brief  Handle the first-run state before entering remote wake mode.
  * @param[in,out] ctx Application state context.
  * @retval Next application state.
  */
static APP_STATE App_HandleFirstRun(APP_CONTEXT *ctx)
{
	if(ctx->remoteModuleOk)
	{
		App_EnterStopAndWaitRemote(ctx);
		return APP_STATE_JUDGE_REMOTE_MSG;
	}

	return APP_STATE_SYS_INIT;
}

/**
  * @brief  Judge whether the received remote wake frame should start runtime.
  * @param[in,out] ctx Application state context.
  * @retval Next application state.
  */
static APP_STATE App_HandleRemoteWake(APP_CONTEXT *ctx)
{
	int length = 0;

	if(uwTick - ctx->wakeTick >= APP_REMOTE_WAKE_TIMEOUT_MS)
	{
		return APP_STATE_FIRST_RUN;
	}

	if(mb_usart3_t.rx_end_flg == 1)
	{
		length = AppUart_LimitFrameLength(MsgFlag3);
		mb_usart3_t.rx_end_flg = 0;
		MsgFlag3 = 0;
		if(App_RemoteCommandRequestsStart(mb_usart3_t.rx_buf, length))
		{
			return APP_STATE_SYS_INIT;
		}
	}

	return APP_STATE_JUDGE_REMOTE_MSG;
}

/**
  * @brief  Initialize peripherals and protocol state for runtime operation.
  * @param[in,out] ctx Application state context.
  * @retval None
  */
static void App_InitRunMode(APP_CONTEXT *ctx)
{
	Wireless_StartRuntimeUarts(ctx->remoteModuleOk);

	uwTick = 0;
	ctx->zigbeeRefreshTick = 0;
	ctx->idleSleepTick = 0;
	ctx->measureFlag = 1;
	ctx->relayMode = 0;
	ctx->bcState = 0;
	ctx->bcResp = 0;
	ctx->lastBroadcastId = 0;
}

/**
  * @brief  Perform deferred all-channel measurement if requested.
  * @param[in,out] ctx Application state context.
  * @retval None
  */
static void App_RunPendingMeasurement(APP_CONTEXT *ctx)
{
	if(ctx->measureFlag)
	{
		ctx->errorCode = CH8_MeasureAllChannels(TestValue, RefferenceData, workMode, kindex.fkvalue, fCorrect);
		ctx->measureFlag = 0;
	}
}

/**
  * @brief  Drop remote frames received while the board is already running.
  * @retval None
  */
static void App_ClearRuntimeRemoteFrame(void)
{
	if(mb_usart3_t.rx_end_flg == 1)
	{
		mb_usart3_t.rx_end_flg = 0;
		MsgFlag3 = 0;
	}
}

/**
  * @brief  Reinitialize Zigbee receive if no frame has arrived for a while.
  * @param[in,out] ctx Application state context.
  * @retval None
  */
static void App_RefreshZigbeeReceiver(APP_CONTEXT *ctx)
{
	if(uwTick - ctx->zigbeeRefreshTick >= APP_ZIGBEE_REINIT_INTERVAL_MS)
	{
		ctx->zigbeeRefreshTick = uwTick;
		MX_USART2_UART_Init(WIRELESS_ZIGBEE_RUNTIME_BAUDRATE);
		HAL_UART_Receive_IT(&huart2, mb_usart2_t.rx_buf, MB_BUF_SIZE);
		mb_usart2_t.rx_end_flg = 0;
		MsgFlag2 = 0;
	}
}

/**
  * @brief  Forward an unhandled Zigbee frame to the VM101 measurement module.
  * @param[in] length Number of bytes to forward.
  * @param[in,out] ctx Application state context.
  * @retval None
  */
static void App_StartVm101Relay(int length, APP_CONTEXT *ctx)
{
	if(length <= 0)
	{
		return;
	}

	mb_usart1_t.rx_end_flg = 0;
	MsgFlag1 = 0;
	ctx->relayMode = 1;
	Usart_Printf_Len(&huart1, RxBuffer2, (uint16_t)length);
}

/**
  * @brief  Process one completed Zigbee frame.
  * @param[in,out] ctx Application state context.
  * @retval 1 if the caller should skip the rest of the loop, otherwise 0.
  */
static u8 App_ProcessZigbeeFrame(APP_CONTEXT *ctx)
{
	int length = 0;
	HY65_ASCII_RESULT asciiResult = HY65_ASCII_NOT_MATCH;
	ZDYH_RESULT zdyhResult = ZDYH_NOT_MATCH;

	if(mb_usart2_t.rx_end_flg != 1)
	{
		return 0;
	}

	ctx->zigbeeRefreshTick = uwTick;
	ctx->idleSleepTick = uwTick;
	length = AppUart_CopyRxData(RxBuffer2, mb_usart2_t.rx_buf, MsgFlag2);
	mb_usart2_t.rx_end_flg = 0;
	MsgFlag2 = 0;

	zdyhResult = ZDYH_Process(RxBuffer2, TxBuffer2, TestValue,
		&ctx->bcState, &ctx->bcResp, &ctx->lastBroadcastId,
		&ctx->lastCmd, ctx->lastMsg,
		&ctx->errorCode, &ctx->measureFlag);
	if(ZDYH_CONTINUE == zdyhResult)
	{
		return 1;
	}
	if(ZDYH_HANDLED == zdyhResult)
	{
		ctx->relayMode = 0;
		return 0;
	}

	asciiResult = HY65_AsciiProcess(RxBuffer2, TxBuffer2, length, TestValue, &ctx->errorCode, &ctx->measureFlag);
	if(HY65_ASCII_CONTINUE == asciiResult)
	{
		return 1;
	}
	if(HY65_ASCII_HANDLED == asciiResult)
	{
		ctx->relayMode = 0;
		return 0;
	}

	App_StartVm101Relay(length, ctx);
	return 0;
}

/**
  * @brief  Forward VM101 relay response back to Zigbee.
  * @param[in,out] ctx Application state context.
  * @retval None
  */
static void App_ProcessVm101RelayResponse(APP_CONTEXT *ctx)
{
	int length = 0;

	if(0 == ctx->relayMode || mb_usart1_t.rx_end_flg != 1)
	{
		return;
	}

	length = AppUart_CopyRxData(TxBuffer2, mb_usart1_t.rx_buf, MsgFlag1);
	mb_usart1_t.rx_end_flg = 0;
	MsgFlag1 = 0;
	if(length > 0)
	{
		Usart_Printf_Len(&huart2, TxBuffer2, (uint16_t)length);
	}
}

/**
  * @brief  Run one iteration of the active measurement and communication state.
  * @param[in,out] ctx Application state context.
  * @retval Next application state.
  */
static APP_STATE App_HandleRunMode(APP_CONTEXT *ctx)
{
	if(uwTick - ctx->idleSleepTick >= APP_IDLE_SLEEP_TIMEOUT_MS)
	{
		return APP_STATE_FIRST_RUN;
	}

	App_RunPendingMeasurement(ctx);
	App_ClearRuntimeRemoteFrame();
	if(App_ProcessZigbeeFrame(ctx))
	{
		return APP_STATE_RUN_MODE;
	}
	App_RefreshZigbeeReceiver(ctx);
	App_ProcessVm101RelayResponse(ctx);

	return APP_STATE_RUN_MODE;
}

/**
  * @brief  Run the CH8 application state machine.
  * @retval None
  */
void App_Run(void)
{
	APP_CONTEXT ctx;

	memset(&ctx, 0, sizeof(ctx));
	ctx.state = APP_STATE_FIRST_RUN;
	ctx.errorCode = 1;
	ctx.lastCmd = 0x80;

	AppConfig_Load();
	AppUart_InitContexts();
	ctx.remoteModuleOk = App_CheckWirelessAtStartup();

	while(1)
	{
		switch(ctx.state)
		{
			case APP_STATE_FIRST_RUN:
				ctx.state = App_HandleFirstRun(&ctx);
				break;
			case APP_STATE_JUDGE_REMOTE_MSG:
				ctx.state = App_HandleRemoteWake(&ctx);
				break;
			case APP_STATE_SYS_INIT:
				App_InitRunMode(&ctx);
				ctx.state = APP_STATE_RUN_MODE;
				break;
			case APP_STATE_RUN_MODE:
				ctx.state = App_HandleRunMode(&ctx);
				break;
			default:
				ctx.state = APP_STATE_FIRST_RUN;
				break;
		}
	}
}
