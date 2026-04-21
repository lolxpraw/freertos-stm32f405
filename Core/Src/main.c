/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "LCD_Driver.h"
#include "stdio.h"
#include "tmpsensor.h"
#include "touch.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define WAV_HEADER_SIZE        44
#define AUDIO_BUFFER_SAMPLES   4096
#define AUDIO_HALF_SAMPLES     (AUDIO_BUFFER_SAMPLES / 2)
#define AUDIO_SAMPLE_RATE      16000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

DAC_HandleTypeDef hdac;
DMA_HandleTypeDef hdma_dac1;

SD_HandleTypeDef hsd;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
FIL musicFile;

uint16_t dacBuffer[AUDIO_BUFFER_SAMPLES];
uint8_t wavTemp[AUDIO_HALF_SAMPLES];
uint8_t wavHeader[WAV_HEADER_SIZE];

volatile uint8_t sd_ready = 0;
volatile uint8_t audio_playing = 0;
volatile uint8_t audio_half_req = 0;
volatile uint8_t audio_full_req = 0;
volatile uint8_t music_file_opened = 0;
volatile UINT wavDataOffset = WAV_HEADER_SIZE;

volatile uint32_t audio_total_samples = 0;
volatile uint32_t audio_played_samples = 0;
//uint32_t adc_value;
//float temperature;

//Khang's code start
typedef struct AdcValues{
uint16_t Raw[2]; /* Raw values from ADC */
double IntSensTmp; /* Temperature */
}adcval_t;
adcval_t Adc;
typedef struct Flags
{
uint8_t ADCCMPLT;
}flag_t;
flag_t Flg = {0, };
//Khang's code end
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM3_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_DAC_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */
float Read_Temperature(void);
uint8_t Audio_Start(void);
void Audio_Stop(void);
void Audio_Service(void);
void Audio_FillHalf(uint16_t startIndex);
uint16_t WAV_ReadU16(const uint8_t *p);
uint32_t WAV_ReadU32(const uint8_t *p);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t WAV_ReadU16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

uint32_t WAV_ReadU32(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

void Audio_FillHalf(uint16_t startIndex)
{
    UINT br = 0;
    UINT total = 0;
    uint16_t i;

    while (total < AUDIO_HALF_SAMPLES)
    {
        if (f_read(&musicFile, &wavTemp[total], AUDIO_HALF_SAMPLES - total, &br) != FR_OK)
        {
            br = 0;
        }

        if (br == 0)
        {
            f_lseek(&musicFile, wavDataOffset);   // quay lại đúng đầu data
            continue;
        }

        total += br;
    }

    for (i = 0; i < AUDIO_HALF_SAMPLES; i++)
    {
        dacBuffer[startIndex + i] = ((uint16_t)wavTemp[i]) << 4;   // 8-bit unsigned -> 12-bit DAC
    }
}
uint8_t Audio_Start(void)
{
    UINT scanRead;
    FRESULT fres;
    char dbg[40];
    uint16_t i;
    uint8_t scanBuf[256];

    if (!sd_ready)
    {
        lcd_fill_rect(15, 185, 220, 25, WHITE);
        lcd_display_string(20, 190, (uint8_t *)"ERR: SD not ready", FONT_1206, RED);
        return 0;
    }

    if (audio_playing)
        return 1;

    fres = f_open(&musicFile, "music.wav", FA_READ);
    if (fres != FR_OK)
    {
        sprintf(dbg, "ERR open: %d", fres);
        lcd_fill_rect(15, 185, 220, 25, WHITE);
        lcd_display_string(20, 190, (uint8_t *)dbg, FONT_1206, RED);
        return 0;
    }

    music_file_opened = 1;

    fres = f_read(&musicFile, scanBuf, sizeof(scanBuf), &scanRead);
    if (fres != FR_OK || scanRead < 44)
    {
        sprintf(dbg, "ERR read: %d/%u", fres, scanRead);
        lcd_fill_rect(15, 185, 220, 25, WHITE);
        lcd_display_string(20, 190, (uint8_t *)dbg, FONT_1206, RED);
        f_close(&musicFile);
        music_file_opened = 0;
        return 0;
    }

    if (memcmp(&scanBuf[0], "RIFF", 4) != 0)
    {
        lcd_fill_rect(15, 185, 220, 25, WHITE);
        lcd_display_string(20, 190, (uint8_t *)"ERR: no RIFF", FONT_1206, RED);
        f_close(&musicFile);
        music_file_opened = 0;
        return 0;
    }

    if (memcmp(&scanBuf[8], "WAVE", 4) != 0)
    {
        lcd_fill_rect(15, 185, 220, 25, WHITE);
        lcd_display_string(20, 190, (uint8_t *)"ERR: no WAVE", FONT_1206, RED);
        f_close(&musicFile);
        music_file_opened = 0;
        return 0;
    }

    if (memcmp(&scanBuf[12], "fmt ", 4) != 0)
    {
        lcd_fill_rect(15, 185, 220, 25, WHITE);
        lcd_display_string(20, 190, (uint8_t *)"ERR: no fmt", FONT_1206, RED);
        f_close(&musicFile);
        music_file_opened = 0;
        return 0;
    }

    if (WAV_ReadU16(&scanBuf[20]) != 1)
    {
        sprintf(dbg, "ERR fmt=%u", WAV_ReadU16(&scanBuf[20]));
        lcd_fill_rect(15, 185, 220, 25, WHITE);
        lcd_display_string(20, 190, (uint8_t *)dbg, FONT_1206, RED);
        f_close(&musicFile);
        music_file_opened = 0;
        return 0;
    }

    if (WAV_ReadU16(&scanBuf[22]) != 1)
    {
        sprintf(dbg, "ERR ch=%u", WAV_ReadU16(&scanBuf[22]));
        lcd_fill_rect(15, 185, 220, 25, WHITE);
        lcd_display_string(20, 190, (uint8_t *)dbg, FONT_1206, RED);
        f_close(&musicFile);
        music_file_opened = 0;
        return 0;
    }

    if (WAV_ReadU32(&scanBuf[24]) != 16000)
    {
        sprintf(dbg, "ERR rate=%lu", WAV_ReadU32(&scanBuf[24]));
        lcd_fill_rect(15, 185, 220, 25, WHITE);
        lcd_display_string(20, 190, (uint8_t *)dbg, FONT_1206, RED);
        f_close(&musicFile);
        music_file_opened = 0;
        return 0;
    }

    if (WAV_ReadU16(&scanBuf[34]) != 8)
    {
        sprintf(dbg, "ERR bits=%u", WAV_ReadU16(&scanBuf[34]));
        lcd_fill_rect(15, 185, 220, 25, WHITE);
        lcd_display_string(20, 190, (uint8_t *)dbg, FONT_1206, RED);
        f_close(&musicFile);
        music_file_opened = 0;
        return 0;
    }

    wavDataOffset = 0;
    for (i = 12; (i + 8) < scanRead; i++)
    {
        if (memcmp(&scanBuf[i], "data", 4) == 0)
        {
            wavDataOffset = i + 8;   // sau "data" + size
            break;
        }
    }

    if (wavDataOffset == 0)
    {
        lcd_fill_rect(15, 185, 220, 25, WHITE);
        lcd_display_string(20, 190, (uint8_t *)"ERR: no data", FONT_1206, RED);
        f_close(&musicFile);
        music_file_opened = 0;
        return 0;
    }

    if (f_lseek(&musicFile, wavDataOffset) != FR_OK)
    {
        lcd_fill_rect(15, 185, 220, 25, WHITE);
        lcd_display_string(20, 190, (uint8_t *)"ERR: seek data", FONT_1206, RED);
        f_close(&musicFile);
        music_file_opened = 0;
        return 0;
    }

    /* Tinh tong so samples = (file_size - header_offset) / 1 byte per sample */
    uint32_t file_size = f_size(&musicFile);
    if (file_size > wavDataOffset)
        audio_total_samples = file_size - wavDataOffset;
    else
        audio_total_samples = 0;
    audio_played_samples = 0;

    Audio_FillHalf(0);
    Audio_FillHalf(AUDIO_HALF_SAMPLES);

    audio_half_req = 0;
    audio_full_req = 0;

    HAL_TIM_Base_Start(&htim6);
    HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)dacBuffer, AUDIO_BUFFER_SAMPLES, DAC_ALIGN_12B_R);

    audio_playing = 1;
    return 1;
}

void Audio_Stop(void)
{
    if (audio_playing)
    {
        HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
        HAL_TIM_Base_Stop(&htim6);
        audio_playing = 0;
    }

    if (music_file_opened)
    {
        f_close(&musicFile);
        music_file_opened = 0;
    }
}

void Audio_Service(void)
{
    if (!audio_playing)
        return;

    if (audio_half_req)
    {
        audio_half_req = 0;
        audio_played_samples += AUDIO_HALF_SAMPLES;
        Audio_FillHalf(0);
    }

    if (audio_full_req)
    {
        audio_full_req = 0;
        audio_played_samples += AUDIO_HALF_SAMPLES;
        Audio_FillHalf(AUDIO_HALF_SAMPLES);
    }
}

void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    audio_half_req = 1;
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    audio_full_req = 1;
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_SPI1_Init();
  MX_TIM3_Init();
  MX_SDIO_SD_Init();
  MX_USART1_UART_Init();
  MX_FATFS_Init();
  MX_DAC_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  FRESULT res;
  HAL_StatusTypeDef hsd_res;

  sd_ready = 0;

  lcd_init();
  tp_init();

  lcd_clear_screen(0x2B4F);

  /* Title */
  lcd_fill_rect(10, 10, 100, 35, WHITE);
  lcd_display_string(22, 20, (uint8_t *)"[Nhom 09]", FONT_1608, BLACK);

  /* Internal Temp */
  lcd_fill_rect(10, 55, 220, 30, WHITE);
  lcd_display_string(20, 64, (uint8_t *)"Internal Temperature:", FONT_1206, BLACK);

  /* Nut Play/Pause */
  lcd_fill_rect(10, 90, 220, 80, WHITE);
  lcd_draw_line(50, 105, 90, 130, BLACK);
  lcd_draw_line(90, 130, 50, 155, BLACK);
  lcd_draw_line(50, 155, 50, 105, BLACK);
  lcd_fill_rect(130, 105, 18, 50, BLACK);
  lcd_fill_rect(165, 105, 18, 50, BLACK);

  /* Data */
  lcd_fill_rect(10, 180, 220, 130, WHITE);
  lcd_display_string(20, 190, (uint8_t *)"Data...", FONT_1206, BLACK);

  /* Khoi tao SD */
  hsd_res = HAL_SD_Init(&hsd);
  if (hsd_res != HAL_OK)
  {
      lcd_display_string(20, 210, (uint8_t*)"HAL_SD_Init FAIL", FONT_1206, RED);
  }
  else
  {
      /* chuyen tu 1-bit sang 4-bit */
      hsd_res = HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B);

      if (hsd_res != HAL_OK)
      {
          lcd_display_string(20, 210, (uint8_t*)"4-bit config FAIL", FONT_1206, RED);
      }
      else
      {
          HAL_Delay(20);
          res = f_mount(&SDFatFS, SDPath, 1);

          if (res == FR_OK)
          {
              sd_ready = 1;
              lcd_display_string(20, 210, (uint8_t*)"SD OK", FONT_1206, GREEN);

              /* kiem tra co music.wav khong */
              res = f_open(&musicFile, "music.wav", FA_READ);
              if (res == FR_OK)
              {
                  f_close(&musicFile);
                  lcd_fill_rect(20, 230, 200, 20, WHITE);
                  lcd_display_string(20, 230, (uint8_t*)"music.wav READY", FONT_1206, GREEN);
              }
              else
              {
                  lcd_fill_rect(20, 230, 200, 20, WHITE);
                  lcd_display_string(20, 230, (uint8_t*)"music.wav FAIL", FONT_1206, RED);
              }
          }
          else
          {
              char msg[32];
              sprintf(msg, "Mount FAIL: %d", res);
              lcd_display_string(20, 210, (uint8_t*)msg, FONT_1206, RED);
          }
      }
  }

  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)Adc.Raw, 2);
  HAL_TIM_Base_Start(&htim3);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t last_temp_update = 0;
  uint32_t last_time_update = 0;

  while (1)
  {
      Audio_Service();

      /* --- HIEN THI THOI GIAN BAI NHAC --- */
      if (audio_playing && audio_total_samples > 0 &&
          HAL_GetTick() - last_time_update > 500)
      {
          uint32_t played = audio_played_samples;
          uint32_t total  = audio_total_samples;

          uint32_t cur_s = (played % total) / AUDIO_SAMPLE_RATE;
          uint32_t tot_s = total / AUDIO_SAMPLE_RATE;

          char tbuf[32];
          sprintf(tbuf, "%02lu:%02lu / %02lu:%02lu",
                  (unsigned long)(cur_s / 60), (unsigned long)(cur_s % 60),
                  (unsigned long)(tot_s / 60), (unsigned long)(tot_s % 60));

          lcd_fill_rect(15, 273, 200, 15, WHITE);
          lcd_display_string(20, 275, (uint8_t *)tbuf, FONT_1206, BLACK);

          last_time_update = HAL_GetTick();
      }

      /* --- PHẦN 1: CẬP NHẬT NHIỆT ĐỘ --- */
      if (Flg.ADCCMPLT)
      {
          //if (HAL_GetTick() - last_temp_update > 1000)
    	  if (HAL_GetTick() - last_temp_update > (audio_playing ? 2000 : 1000))
          {
              char buf[32];
              int temp_int, temp_frac;

              Adc.IntSensTmp = TMPSENSOR_getTemperature(Adc.Raw[1], Adc.Raw[0]);
              temp_int = (int)Adc.IntSensTmp;
              temp_frac = (int)((Adc.IntSensTmp - temp_int) * 100.0f);
              sprintf(buf, "%d.%02d C", temp_int, temp_frac);

              lcd_fill_rect(150, 64, 70, 12, WHITE);
              lcd_display_string(155, 64, (uint8_t *)buf, FONT_1206, BLACK);

              last_temp_update = HAL_GetTick();
          }
          Flg.ADCCMPLT = 0;
      }

      /* --- PHẦN 2: XỬ LÝ CẢM ỨNG (DÙNG GIÁ TRỊ RAW) --- */
      uint16_t raw_x = 0;
      uint16_t raw_y = 0;

      if (Get_Touch_XY(&raw_x, &raw_y))
      {
//          char dbg_buf[32];
//          sprintf(dbg_buf, "Raw X:%04d Y:%04d", raw_x, raw_y);
//          lcd_fill_rect(10, 280, 200, 20, WHITE);
//          lcd_display_string(10, 280, (uint8_t *)dbg_buf, FONT_1206, RED);

          /* Khung nut Play/Pause */
          if (raw_x >= 1900 && raw_x <= 2700 && raw_y >= 1000 && raw_y <= 2900)
          {
              if (raw_y >= 1900)   /* PLAY */
              {
                  lcd_fill_rect(15, 185, 200, 25, WHITE);

                  if (Audio_Start())
                  {
                      lcd_display_string(20, 190, (uint8_t *)"Nhac: PLAYING...", FONT_1206, BLACK);
                  }
//                  else
//                  {
//                      lcd_display_string(20, 190, (uint8_t *)"music.wav FAIL", FONT_1206, RED);
//                  }

                  //HAL_Delay(300);
                  //HAL_Delay(30);
              }
              else                  /* PAUSE */
              {
                  Audio_Stop();
                  lcd_fill_rect(15, 185, 200, 25, WHITE);
                  lcd_display_string(20, 190, (uint8_t *)"Nhac: PAUSED...", FONT_1206, BLACK);
                  lcd_fill_rect(15, 273, 200, 15, WHITE);   // xoa dong thoi gian
                  HAL_Delay(300);
              }
          }
      }
  }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{

  /* USER CODE BEGIN DAC_Init 0 */

  /* USER CODE END DAC_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC_Init 1 */

  /* USER CODE END DAC_Init 1 */

  /** DAC Initialization
  */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC_Init 2 */

  /* USER CODE END DAC_Init 2 */

}

/**
  * @brief SDIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDIO_SD_Init(void)
{

  /* USER CODE BEGIN SDIO_Init 0 */

  /* USER CODE END SDIO_Init 0 */

  /* USER CODE BEGIN SDIO_Init 1 */

  /* USER CODE END SDIO_Init 1 */
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv = 118;
  /* USER CODE BEGIN SDIO_Init 2 */

  /* USER CODE END SDIO_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 840-1;
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
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 41;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 124;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_RST_Pin|LCD_BL_Pin|LCD_CS_Pin|LCD_DC_Pin
                          |TP_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LCD_RST_Pin LCD_BL_Pin LCD_CS_Pin LCD_DC_Pin
                           TP_CS_Pin */
  GPIO_InitStruct.Pin = LCD_RST_Pin|LCD_BL_Pin|LCD_CS_Pin|LCD_DC_Pin
                          |TP_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : TP_IRQ_Pin */
  GPIO_InitStruct.Pin = TP_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(TP_IRQ_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
float Read_Temperature(void)
{
    uint32_t adc_temp, adc_vref;
    float Vref, Vsense, temperature;

    HAL_ADC_Start(&hadc1);

    // Đọc nhiệt độ
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    adc_temp = HAL_ADC_GetValue(&hadc1);

    // Đọc Vref
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    adc_vref = HAL_ADC_GetValue(&hadc1);

    // Tính lại điện áp tham chiếu
    Vref = 1.21 * 4095 / adc_vref;

    Vsense = adc_temp * Vref / 4095;

    temperature = (Vsense - 0.76) / 0.0025 + 25;

    return temperature;
}
//Khang's code start
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1) /* Check if the interrupt comes from ACD1 */
    {
    /* Set flag to true */
    Flg.ADCCMPLT = 255;
    }
}
//Khang's code end
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
