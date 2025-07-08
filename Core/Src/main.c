/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "liquidcrystal_i2c.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DHT11_PORT GPIOB
#define DHT11_PIN GPIO_PIN_9
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
char rxBuffer[50];  // Bộ đệm nhận dữ liệu
char txBuffer[100]; // Bộ đệm gửi dữ liệu
char rxChar;  // Biến lưu từng ký tự nhận
uint8_t rxIndex = 0; // Vị trí trong rxBuffer
volatile uint8_t dataReady = 0; // Cờ báo dữ liệu đã sẵn sàng
int16_t currentAngle_P = 0;
int16_t currentAngle_A = 0;
volatile char pillType;
volatile int quantity;
int16_t currentAngle = 0;
uint8_t RHI, RHD, TCI, TCD, SUM;
uint32_t pMillis, cMillis;
float tCelsius = 0;
float tFahrenheit = 0;
float RH = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
void ARM_Process(char pillType, int quantity);
void Set_Servo_Angle(TIM_HandleTypeDef *htim, uint32_t channel, uint8_t angle);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void microDelay (uint16_t delay)
{
  __HAL_TIM_SET_COUNTER(&htim3, 0);
  while (__HAL_TIM_GET_COUNTER(&htim3) < delay);
}

uint8_t DHT11_Start (void)
{
  uint8_t Response = 0;
  GPIO_InitTypeDef GPIO_InitStructPrivate = {0};
  GPIO_InitStructPrivate.Pin = DHT11_PIN;
  GPIO_InitStructPrivate.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructPrivate.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStructPrivate.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStructPrivate); // set the pin as output
  HAL_GPIO_WritePin (DHT11_PORT, DHT11_PIN, 0);   // pull the pin low
  HAL_Delay(20);   // wait for 20ms
  HAL_GPIO_WritePin (DHT11_PORT, DHT11_PIN, 1);   // pull the pin high
  microDelay (30);   // wait for 30us
  GPIO_InitStructPrivate.Mode = GPIO_MODE_INPUT;
  GPIO_InitStructPrivate.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStructPrivate); // set the pin as input
  microDelay (40);
  if (!(HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)))
  {
    microDelay (80);
    if ((HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN))) Response = 1;
  }
  pMillis = HAL_GetTick();
  cMillis = HAL_GetTick();
  while ((HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)) && pMillis + 2 > cMillis)
  {
    cMillis = HAL_GetTick();
  }
  return Response;
}

uint8_t DHT11_Read (void)
{
  uint8_t a,b;
  for (a=0;a<8;a++)
  {
    pMillis = HAL_GetTick();
    cMillis = HAL_GetTick();
    while (!(HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)) && pMillis + 2 > cMillis)
    {  // wait for the pin to go high
      cMillis = HAL_GetTick();
    }
    microDelay (40);   // wait for 40 us
    if (!(HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)))   // if the pin is low
      b&= ~(1<<(7-a));
    else
      b|= (1<<(7-a));
    pMillis = HAL_GetTick();
    cMillis = HAL_GetTick();
    while ((HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)) && pMillis + 2 > cMillis)
    {  // wait for the pin to go low
      cMillis = HAL_GetTick();
    }
  }
  return b;
}
// Hàm gửi dữ liệu qua UART
void UART_Send(const char* message)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message), HAL_MAX_DELAY);
}

// Bắt đầu nhận từng byte qua UART
void UART_Receive(void)
{
    HAL_UART_Receive_IT(&huart1, (uint8_t*)&rxChar, 1); // Nhận từng ký tự
}

// Hàm xử lý dữ liệu nhận được từ UART
void ProcessReceivedData(void)
{
    if (sscanf(rxBuffer, "%c%d", &pillType, &quantity) == 2)  // Đọc dữ liệu thành công
    {
            sprintf(txBuffer, "Pill Type: %c, Quantity: %d\r\n", pillType, quantity);
            UART_Send(txBuffer);
            ARM_Process(pillType,quantity);
    }
    rxIndex = 0;  // Reset vị trí buffer
}
void ARM_Process(char pillType, int quantity) {
	switch (pillType) {
	   case 'A':
		   for(int i=quantity;i>0;i--){
			   LCD_Clear();
			   HAL_Delay(100);
			   LCD_SetCursor(0, 0);
			   LCD_PrintStr("Thuoc dau bung");
			   LCD_SetCursor(0, 1);
			   char buffer[16];
			   sprintf(buffer, "So luong: %d", quantity); // Hiển thị số lượng
			   LCD_PrintStr(buffer);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_3, 90);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_2, 135);
			   HAL_Delay(1000);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_1, 180);
			   HAL_Delay(1000);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_3, 115);
			   HAL_Delay(1000);
			   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
			   HAL_Delay(5000);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_2, 0);
			   HAL_Delay(1000);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_3, 90);
			   HAL_Delay(1000);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_1, 0);
			   HAL_Delay(1000);
			   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
			   HAL_Delay(2000);
			   LCD_Clear();
			   HAL_Delay(20);
			   LCD_SetCursor(0, 0);
			   LCD_PrintStr("Thanh cong");
			   HAL_Delay(1000);
		   }
	              // Gọi hàm di chuyển tay robot kiểu A
	            break;
	   case 'B':
		   for(int i=quantity;i>0;i--){
			   LCD_Clear();
			   HAL_Delay(100);
			   LCD_SetCursor(0, 0);
			   LCD_PrintStr("Thuoc ha sot");
			   LCD_SetCursor(0, 1);
			   char buffer[16];
			   sprintf(buffer, "So luong: %d", quantity); // Hiển thị số lượng
			   LCD_PrintStr(buffer);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_3, 90);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_2, 135);
			   HAL_Delay(1000);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_1, 135);
			   HAL_Delay(1000);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_3, 115);
			   HAL_Delay(1000);
			   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
			   HAL_Delay(5000);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_2, 0);
			   HAL_Delay(1000);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_3, 90);
			   HAL_Delay(1000);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_1, 0);
			   HAL_Delay(1000);
			   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
			   HAL_Delay(2000);
			   LCD_Clear();
			   HAL_Delay(20);
			   LCD_SetCursor(0, 0);
			   LCD_PrintStr("Thanh cong");
			   HAL_Delay(1000);
		   }
	              // Gọi hàm di chuyển tay robot kiểu B
	            break;
	   case 'C':
		   for(int i=quantity;i>0;i--){
			   LCD_Clear();
			   HAL_Delay(100);
			   LCD_SetCursor(0, 0);
			   LCD_PrintStr("Thuoc dau dau");
			   LCD_SetCursor(0, 1);
			   char buffer[16];
			   sprintf(buffer, "So luong: %d", quantity); // Hiển thị số lượng
			   LCD_PrintStr(buffer);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_3, 90);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_2, 135);
			   HAL_Delay(1000);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_1, 90);
			   HAL_Delay(1000);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_3, 115);
			   HAL_Delay(1000);
			   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
			   HAL_Delay(5000);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_2, 0);
			   HAL_Delay(1000);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_3, 90);
			   HAL_Delay(1000);
			   Set_Servo_Angle(&htim2, TIM_CHANNEL_1, 0);
			   HAL_Delay(1000);
			   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
			   HAL_Delay(2000);
			   LCD_Clear();
			   HAL_Delay(20);
			   LCD_SetCursor(0, 0);
			   LCD_PrintStr("Thanh cong");
			   HAL_Delay(1000);
		   }
	            // Gọi hàm di chuyển tay robot kiểu C
	            break;
	    }
	UART_Send("Process Finished");
	}
void Set_Servo_Angle(TIM_HandleTypeDef *htim, uint32_t channel, uint8_t angle) {
    // Độ rộng xung từ 1ms đến 2ms tương ứng với góc từ 0 đến 180
    uint16_t pulse = (uint16_t)(1000 + (angle * 1000) / 180); // Tính pulse từ góc

    // Cập nhật độ rộng xung cho kênh PWM
    __HAL_TIM_SET_COMPARE(htim, channel, pulse);
}
// Hàm ngắt nhận từng ký tự UART

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        if (rxChar == '\n')  // Khi nhận đủ một chuỗi (kết thúc bằng '\n')
        {
            rxBuffer[rxIndex] = '\0';  // Đánh dấu kết thúc chuỗi
            dataReady = 1;  // Đánh dấu dữ liệu đã sẵn sàng
        }
        else
        {
            rxBuffer[rxIndex++] = rxChar;  // Lưu ký tự vào buffer
            if (rxIndex >= sizeof(rxBuffer)) // Tránh tràn bộ nhớ
            {
                rxIndex = 0;
            }
        }

        // Tiếp tục nhận ký tự tiếp theo
        UART_Receive();
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_I2C1_Init();
  LCD_Init(2);
  LCD_Backlight();
  LCD_Display();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_3);
  UART_Receive();


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
	  Set_Servo_Angle(&htim2, TIM_CHANNEL_2, 0);
	 	  HAL_Delay(1000);
	  Set_Servo_Angle(&htim2, TIM_CHANNEL_1, 0);
	 	  HAL_Delay(1000);
	  Set_Servo_Angle(&htim2, TIM_CHANNEL_3, 90);
	  if (dataReady)  // Kiểm tra cờ báo dữ liệu đã sẵn sàng
	  	 {
	  	  ProcessReceivedData();
	  	  dataReady = 0;  // Reset lại cờ báo
	  	 }
	  if(DHT11_Start())
	       {
	         RHI = DHT11_Read(); // Relative humidity integral
	         RHD = DHT11_Read(); // Relative humidity decimal
	         TCI = DHT11_Read(); // Celsius integral
	         TCD = DHT11_Read(); // Celsius decimal
	         SUM = DHT11_Read(); // Check sum
	         char snum1[4];
	         char snum2[4];
	         // Cập nhật mã này
	         sprintf(snum1, "%d", TCI); // Chuyển đổi số nguyên TCI thành chuỗi snum1
	         sprintf(snum2, "%d", RHI); // Chuyển đổi số nguyên RHI thành chuỗi snum2
	         LCD_Clear();
	         LCD_Clear(); // Xóa màn hình LCD
	         LCD_SetCursor(6, 1); // Đặt con trỏ tại dòng 0, cột 0
	         LCD_PrintStr("T:"); // In ra "Nhiet do:"
	         LCD_SetCursor(8, 1); // Đặt con trỏ tại dòng 0, cột 10
	         LCD_PrintStr(snum1); // In nhiệt độ
	         LCD_SetCursor(12, 1); // Đặt con trỏ tại dòng 0, cột 0
			 LCD_PrintStr("H:"); // In ra "Nhiet do:"
			 LCD_SetCursor(14, 1); // Đặt con trỏ tại dòng 0, cột 10
			 LCD_PrintStr(snum1); // In nhiệt độ

	        }
	  HAL_Delay(1000);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 72-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 20000-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 72-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 10000-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
