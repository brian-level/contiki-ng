/*
 * Copyright (C) 2022 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup gecko
 * @{
 *
 * \addtogroup gecko-dev Device drivers
 * @{
 *
 * \addtogroup gecko-uart UART driver
 * @{
 *
 * \file
 *         UART implementation for the gecko series 2
 * \author
 *         Brian Dodge <bdodge09@gmail.com>
 *
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "uart-arch.h"
#if UART_USE_IOSTREAM
#include "sl_iostream_init_eusart_instances.h"
#include "sl_iostream_init_instances.h"
#include "sl_iostream.h"
/*---------------------------------------------------------------------------*/
static int (*input_handler)(unsigned char c) = NULL;
/*---------------------------------------------------------------------------*/
void
uart_poll_rx(void)
{
  uint8_t rx_buffer[32];
  size_t gotten;
  int i = 0;

  if(input_handler) {
    sl_iostream_read(sl_iostream_get_default(), rx_buffer, sizeof(rx_buffer), &gotten);
    if (gotten > 0) {
      if (UART_ECHO) {
        uart_write(rx_buffer, gotten);
      }
      while (gotten-- > 0) {
        input_handler(rx_buffer[i++]);
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
void
uart_write(unsigned char *s, unsigned int len)
{
   sl_iostream_write(sl_iostream_get_default(), s, len);
}
/*---------------------------------------------------------------------------*/
void
uart_set_input(int (*input)(unsigned char c))
{
  input_handler = input;
}
/*---------------------------------------------------------------------------*/
void
uart_init(void)
{
  sl_iostream_eusart_init_instances();
}
#else
#include "uartdrv.h"
#include "sl_uartdrv_instances.h"
/*---------------------------------------------------------------------------*/
static int (*input_handler)(unsigned char c) = NULL;
#if UART_USE_LDMA
static uint8_t in_a[1], in_b[1];
/*---------------------------------------------------------------------------*/
void
uart_poll_rx(void)
{
}
/*---------------------------------------------------------------------------*/
static void
receive_callback(UARTDRV_HandleData_t *handle, Ecode_t transferStatus,
                 uint8_t *data, UARTDRV_Count_t transferCount)
{
  uint8_t rxData;
  if(transferStatus == ECODE_EMDRV_UARTDRV_OK && input_handler != NULL) {
    rxData = *data;
    UARTDRV_Receive(sl_uartdrv_usart_vcom_handle, data, sizeof(in_a), receive_callback);
    input_handler(rxData);
    if (UART_ECHO) {
      uart_write(&rxData, 1);
    }
  }
}
#else
#include "sl_uartdrv_usart_vcom_config.h"
#include "int-master.h"
/*---------------------------------------------------------------------------*/
static uint8_t rx_data[32];
static int rxhead, rxtail, rxcount, rxsize = sizeof(rx_data);
/*---------------------------------------------------------------------------*/
void
uart_poll_rx(void)
{
  if(input_handler) {
    while (rxcount > 0) {
      if (UART_ECHO) {
        uart_write(&rx_data[rxtail], 1);
      }
      input_handler(rx_data[rxtail]);
      int_master_status_t is = int_master_read_and_disable();
      rxtail++;
      if(rxtail >= rxsize) {
        rxtail = 0;
      }
      rxcount--;
      int_master_status_set(is);
    }
  }
}
/*---------------------------------------------------------------------------*/
void USART0_RX_IRQHandler(void)
{
  /* Check for RX data valid interrupt */
  if (USART0->STATUS & USART_STATUS_RXDATAV)
  {
    uint8_t rxData = USART_Rx(USART0);
    USART_IntClear(USART0, USART_IF_RXDATAV);
    rx_data[rxhead++] = rxData;
    if (rxhead >= rxsize) {
      rxhead = 0;
    }
    if (rxcount < rxsize) {
      rxcount++;
    }
  }
}
#endif
/*---------------------------------------------------------------------------*/
void
uart_write(unsigned char *s, unsigned int len)
{
#if UART_USE_LDMA
  UARTDRV_ForceTransmit(sl_uartdrv_usart_vcom_handle, s, len);
#else
  for(unsigned int i = 0; i < len; i++) {
    USART_Tx(USART0, s[i]);
    while (!(USART0->STATUS & USART_STATUS_TXC))
      ;
    /*
    if(s[i] == '\n') {
       USART_Tx(USART0, '\r');
       while (!(USART0->STATUS & USART_STATUS_TXC))
         ;
    }
    */
  }
#endif
}
/*---------------------------------------------------------------------------*/
void
uart_set_input(int (*input)(unsigned char c))
{
  input_handler = input;

  if(input) {
#if UART_USE_LDMA
    UARTDRV_Receive(sl_uartdrv_usart_vcom_handle, in_a, sizeof(in_a), receive_callback);
    UARTDRV_Receive(sl_uartdrv_usart_vcom_handle, in_b, sizeof(in_b), receive_callback);
#else
    rxhead = rxtail = rxcount = 0;

    /* usart cant actively Rx unless in em1 or less */
    sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);

    /* hook interrupt */
    USART_IntClear(USART0, USART_IF_RXDATAV);
    USART_IntEnable(USART0, USART_IF_RXDATAV);
    NVIC_ClearPendingIRQ(USART0_RX_IRQn);
    NVIC_EnableIRQ(USART0_RX_IRQn);

    /* enable rx */
    USART0->CMD = USART_CMD_RXEN;
#endif
  }
}
/*---------------------------------------------------------------------------*/
void
uart_init(void)
{
  sl_uartdrv_init_instances();
}
#endif
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */
