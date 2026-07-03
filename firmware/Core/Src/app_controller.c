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
  * @brief  闪烁状态灯，表示遥控模块可用。
  * @retval 无
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
  * @brief  上电启动阶段给无线模块供电并检查模块状态。
  * @retval 1 表示遥控模块可用，0 表示不可用。
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
  * @brief  Stop 模式唤醒后恢复时钟和 LPUART 接收。
  * @retval 无
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
  * @brief  进入 Stop 模式，并打开 LPUART 接收等待遥控唤醒数据。
  * @param[in,out] ctx 应用状态上下文。
  * @retval 无
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
  * @brief  判断 433 MHz 遥控帧是否要求板卡启动。
  * @param[in] rxBuffer 接收到的遥控帧缓冲区。
  * @param[in] length 接收到的帧长度。
  * @retval 1 表示至少有一路通道命令有效，0 表示无启动命令。
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
  * @brief  处理首次运行状态，然后进入遥控唤醒模式。
  * @param[in,out] ctx 应用状态上下文。
  * @retval 下一个应用状态。
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
  * @brief  判断收到的遥控唤醒帧是否应启动运行流程。
  * @param[in,out] ctx 应用状态上下文。
  * @retval 下一个应用状态。
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
  * @brief  初始化运行阶段需要的外设和协议状态。
  * @param[in,out] ctx 应用状态上下文。
  * @retval 无
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
  * @brief  按需执行延迟触发的全通道测量。
  * @param[in,out] ctx 应用状态上下文。
  * @retval 无
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
  * @brief  清除板卡运行期间收到的遥控帧。
  * @retval 无
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
  * @brief  长时间未收到 Zigbee 数据时重新初始化接收。
  * @param[in,out] ctx 应用状态上下文。
  * @retval 无
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
  * @brief  将未被本机处理的 Zigbee 帧转发给 VM101 测量模块。
  * @param[in] length 待转发字节数。
  * @param[in,out] ctx 应用状态上下文。
  * @retval 无
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
  * @brief  处理一帧完整的 Zigbee 数据。
  * @param[in,out] ctx 应用状态上下文。
  * @retval 1 表示调用者应跳过本轮循环后续处理，0 表示继续处理。
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
  * @brief  将 VM101 透传响应转发回 Zigbee。
  * @param[in,out] ctx 应用状态上下文。
  * @retval 无
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
  * @brief  执行运行状态下的一轮测量和通信处理。
  * @param[in,out] ctx 应用状态上下文。
  * @retval 下一个应用状态。
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
  * @brief  运行 CH8 应用状态机。
  * @retval 无
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
