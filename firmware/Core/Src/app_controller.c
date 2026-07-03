#include "app_controller.h"
#include "app_config.h"
#include "app_uart.h"
#include "ch8_measure.h"
#include "hy65_ascii_cmd.h"
#include "usart.h"
#include "wireless_board.h"
#include "zdyh_cmd.h"

#define APP_ZIGBEE_REINIT_INTERVAL_MS    5000U
#define APP_LAST_MSG_SIZE                20U

static u8 TxBuffer2[MB_BUF_SIZE];
static u8 RxBuffer2[MB_BUF_SIZE];

static int TestValue[CH8_TEST_VALUE_SIZE] = {0};

typedef struct
{
	u8 measureFlag;
	u8 relayMode;
	u8 errorCode;
	u8 bcState;
	u8 bcResp;
	u32 lastBroadcastId;
	uint8_t lastCmd;
	uint8_t lastMsg[APP_LAST_MSG_SIZE];
	u32 zigbeeRefreshTick;
} APP_CONTEXT;

static void App_InitRunMode(APP_CONTEXT *ctx)
{
	Wireless_StartRuntimeUarts();

	uwTick = 0;
	ctx->zigbeeRefreshTick = 0;
	ctx->measureFlag = 1;
	ctx->relayMode = 0;
	ctx->bcState = 0;
	ctx->bcResp = 0;
	ctx->lastBroadcastId = 0;
}

static void App_RunPendingMeasurement(APP_CONTEXT *ctx)
{
	if(ctx->measureFlag)
	{
		ctx->errorCode = CH8_MeasureAllChannels(TestValue, RefferenceData, workMode, kindex.fkvalue, fCorrect);
		ctx->measureFlag = 0;
	}
}

static void App_RefreshZigbeeReceiver(APP_CONTEXT *ctx)
{
	if(uwTick - ctx->zigbeeRefreshTick >= APP_ZIGBEE_REINIT_INTERVAL_MS)
	{
		ctx->zigbeeRefreshTick = uwTick;
		MX_USART1_UART_Init(WIRELESS_ZIGBEE_RUNTIME_BAUDRATE);
		HAL_UART_Receive_IT(&huart1, mb_usart1_t.rx_buf, MB_BUF_SIZE);
		mb_usart1_t.rx_end_flg = 0;
		MsgFlag1 = 0;
	}
}

static void App_StartVm101Relay(int length, APP_CONTEXT *ctx)
{
	if(length <= 0)
	{
		return;
	}

	mb_usart2_t.rx_end_flg = 0;
	MsgFlag2 = 0;
	ctx->relayMode = 1;
	Usart_Printf_Len(&huart2, RxBuffer2, (uint16_t)length);
}

static u8 App_ProcessZigbeeFrame(APP_CONTEXT *ctx)
{
	int length = 0;
	HY65_ASCII_RESULT asciiResult = HY65_ASCII_NOT_MATCH;
	ZDYH_RESULT zdyhResult = ZDYH_NOT_MATCH;

	if(mb_usart1_t.rx_end_flg != 1)
	{
		return 0;
	}

	ctx->zigbeeRefreshTick = uwTick;
	length = AppUart_CopyRxData(RxBuffer2, mb_usart1_t.rx_buf, MsgFlag1);
	mb_usart1_t.rx_end_flg = 0;
	MsgFlag1 = 0;

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

static void App_ProcessVm101RelayResponse(APP_CONTEXT *ctx)
{
	int length = 0;

	if(0 == ctx->relayMode || mb_usart2_t.rx_end_flg != 1)
	{
		return;
	}

	length = AppUart_CopyRxData(TxBuffer2, mb_usart2_t.rx_buf, MsgFlag2);
	mb_usart2_t.rx_end_flg = 0;
	MsgFlag2 = 0;
	if(length > 0)
	{
		Usart_Printf_Len(&huart1, TxBuffer2, (uint16_t)length);
	}
}

static void App_RunModeLoop(APP_CONTEXT *ctx)
{
	App_RunPendingMeasurement(ctx);
	if(App_ProcessZigbeeFrame(ctx))
	{
		return;
	}
	App_RefreshZigbeeReceiver(ctx);
	App_ProcessVm101RelayResponse(ctx);
}

void App_Run(void)
{
	APP_CONTEXT ctx;

	memset(&ctx, 0, sizeof(ctx));
	ctx.errorCode = 1;
	ctx.lastCmd = 0x80;

	AppConfig_Load();
	AppUart_InitContexts();
	App_InitRunMode(&ctx);

	while(1)
	{
		App_RunModeLoop(&ctx);
	}
}
