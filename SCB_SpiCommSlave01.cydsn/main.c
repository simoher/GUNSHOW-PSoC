/*******************************************************************************
* File Name: main.c
*
* Version: 1.0
*
* Description:
*  This example project demonstrates the use of an SCB-based SPI slave
*  component. The SCB SPI slave communicates with a UDB-based SPI master,
*  located on the same PSoC but wired together externally. The SPI slave
*  receives a packet with a command from the SPI master to control the
*  RGB LED color. Once the slave updates the LED color, it returns a status
*  packet to the master.
*
********************************************************************************
* Copyright 2015, Cypress Semiconductor Corporation. All rights reserved.
* This software is owned by Cypress Semiconductor Corporation and is protected
* by and subject to worldwide patent and copyright laws and treaties.
* Therefore, you may use this software only as provided in the license agreement
* accompanying the software package from which you obtained this software.
* CYPRESS AND ITS SUPPLIERS MAKE NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* WITH REGARD TO THIS SOFTWARE, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT,
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*******************************************************************************/

#include <main.h>

/* SPIM functions */
static void SPIM_SendCommandPacket(uint32 cmd);
static uint32 SPIM_ReadStatusPacket(void);

/* SPIS functions */
static uint32 SPIS_WaitForCommand(void);
static void SPIS_UpdateStatus(uint32 status);
static void SPIS_CleanupAfterRead(void);

/* Executes command and returns status */
static uint8 ExecuteCommand(uint32 cmd);

/* This dummy buffer used by SPIM when it receives status packet.
* This dummy buffer used by SPIS when it receives command packet.
*/
const uint8 dummyBuffer[PACKET_SIZE] = {0xFFu, 0xFFu, 0xFFu};


/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
*  The main function performs the following actions:
*   1. Turns off RGB LED.
*   2. Sets up SPIS and SPIM components.
*   3. Enables global interrupts.
*   4. SPI master (SPIM) sends command to the SPI slave (SPIS) to control
*      RGB LED.
*   5. SPIS receives the command, updates the LED and returns a status packet.
*   6. SPIM receives the response status packet. If successful, increment to
*      next command. Otherwise send the same command until successful.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
int main()
{
    uint32 cmd;
    uint32 status;

    /* Start SPIS and SPIM components operation */
    SPIS_Start();
    SPIM_Start();

    RGB_LED_OFF;

    CyGlobalIntEnable;

    /* Set command to start with */
    cmd = CMD_SET_RED;

    /* Put data into the slave TX buffer to be transferred while following
    * master access.
    */
    SPIS_UpdateStatus(STS_CMD_FAIL);

    for(;;)
    {
        /* Send packet with command to the slave */
        SPIM_SendCommandPacket(cmd);

        /* Wait for the command from the master, execute it and update status */
        cmd = SPIS_WaitForCommand();
        status = ExecuteCommand(cmd);
        SPIS_UpdateStatus(status);

        /* Send packet to read response */
        if (STS_CMD_DONE == SPIM_ReadStatusPacket())
        {
            /* Next command to execute */
            cmd++;
            if (cmd > CMD_SET_BLUE)
            {
                cmd = CMD_SET_OFF;
            }
        }

        /* Clear slave buffer after response has been read */
        SPIS_CleanupAfterRead();

        /* Delay between commands */
        CyDelay(CMD_TO_CMD_DELAY);
    }
}


/*******************************************************************************
* Function Name: SPIM_SendCommandPacket
********************************************************************************
* Summary:
* SPIM initiates the transmission of a command packet to the SPIS. After
* transfer completion, the dummy bytes sent by the SPIS are cleared from the
* RX buffer.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
static void SPIM_SendCommandPacket(uint32 cmd)
{
    static uint8 mTxBuffer[PACKET_SIZE] = {PACKET_SOP, CMD_SET_BLUE, PACKET_EOP};

    mTxBuffer[PACKET_CMD_POS] = (uint8) cmd;

    /* Start transfer */
    SPIM_PutArray(mTxBuffer, PACKET_SIZE);

    /* Wait for the end of the transfer monitoring the SPI_DONE status */
    while (0u == (SPIM_ReadTxStatus() & SPIM_STS_SPI_DONE))
    {
    }

    /* Clear dummy bytes from RX buffer */
    SPIM_ClearRxBuffer();
}


/*******************************************************************************
* Function Name: SPIM_ReadStatusPacket
********************************************************************************
* Summary:
*  SPIM initiates the transmission of a dummy packet to collect the status
*  information from the SPIS. After the transfer is complete the format of
*  the packet is verified and the status is returned. If the format of the
*  packet is invalid the unknown status is returned.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
static uint32 SPIM_ReadStatusPacket(void)
{
    uint8 tmpBuffer[PACKET_SIZE];
    uint8 status;
    uint32 i;

    /* Start transfer */
    SPIM_PutArray(dummyBuffer, PACKET_SIZE);

    /* Wait for the end of the transfer. Monitor the SPI_DONE status */
    while (0u == (SPIM_ReadTxStatus() & SPIM_STS_SPI_DONE))
    {
    }

    /* Clear dummy bytes from TX buffer */
    SPIM_ClearTxBuffer();

    /* Read data from the RX buffer */
    i = 0u;
    while (0u != SPIM_GetRxBufferSize())
    {
        tmpBuffer[i] = SPIM_ReadRxData();
        i++;
    }

    /* Check packet format */
    if ((tmpBuffer[PACKET_SOP_POS] == PACKET_SOP) &&
        (tmpBuffer[PACKET_EOP_POS] == PACKET_EOP))
    {
        /* Return status */
        status = tmpBuffer[PACKET_STS_POS];
    }
    else
    {
        /* Invalid packet format, return fail status */
        status = STS_CMD_FAIL;
    }

    return (status);
}


/*******************************************************************************
* Function Name: SPIS_WaitForCommand
********************************************************************************
* Summary:
*  SPIS waits for completion of the transfer initiated by the SPIM. After packet
*  reception the format of the packet is verified and the command is returned.
*  If the format of the packet is invalid the unknown command is returned.
*
* Parameters:
*  None
*
* Return:
*  Returns command from the received packet.
*
*******************************************************************************/
static uint32 SPIS_WaitForCommand(void)
{
    uint8 tmpBuffer[PACKET_SIZE];
    uint32 cmd;
    uint32 i;

    /* Wait for the end of the transfer */
    while (PACKET_SIZE != SPIS_SpiUartGetRxBufferSize())
    {
    }

    /* Read packet from the buffer */
    i = 0u;
    while (0u != SPIS_SpiUartGetRxBufferSize())
    {
        tmpBuffer[i] = SPIS_SpiUartReadRxData();
        i++;
    }

    /* Check start and end of packet markers */
    if ((tmpBuffer[PACKET_SOP_POS] == PACKET_SOP) &&
        (tmpBuffer[PACKET_EOP_POS] == PACKET_EOP))
    {
        /* Return command */
        cmd = tmpBuffer[PACKET_CMD_POS];
    }
    else
    {
        /* Incorrect packet format, return unknown command */
        cmd = CMD_SET_UNKNOWN;
    }

    return (cmd);
}


/*******************************************************************************
* Function Name: SPIS_CleanupAfterRead
********************************************************************************
* Summary:
*  SPIS waits for completion of the read transfer initiated by the SPIM. The
*  received packet is discarded as it contains only dummy data. Then the SPIS
*  prepares for the following command packet by putting dummy bytes into the
*  TX buffer.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
static void SPIS_CleanupAfterRead(void)
{
    /* Wait for the end of the transfer */
    while (PACKET_SIZE != SPIS_SpiUartGetRxBufferSize())
    {
    }

    /* Clear RX buffer from dummy bytes */
    SPIS_SpiUartClearRxBuffer();

    /* Put dummy data into TX buffer to be transmitted to SPIM */
    SPIS_SpiUartPutArray(dummyBuffer, PACKET_SIZE);
}


/*******************************************************************************
* Function Name: SPIS_UpdateStatus
********************************************************************************
* Summary:
*  SPIS copies packet with response into the buffer.
*
* Parameters:
*  status - status to insert into the response packet.
*
* Return:
*  None
*
*******************************************************************************/
static void SPIS_UpdateStatus(uint32 status)
{
    static uint8 sTxBuffer[PACKET_SIZE] = {PACKET_SOP, STS_CMD_FAIL, PACKET_EOP};

    sTxBuffer[PACKET_STS_POS] = (uint8) status;

    /* Put data into the slave TX buffer to be transferred while following
    * master access.
    */
    SPIS_SpiUartPutArray(sTxBuffer, PACKET_SIZE);
}


/*******************************************************************************
* Function Name: ExecuteCommand
********************************************************************************
* Summary:
*  Executes received command to control the LED color and returns status.
*  If the command is unknown, the LED color is not changed.
*
* Parameters:
*  cmd: command to execute. Available commands:
*   - CMD_SET_RED:   set red color of the LED.
*   - CMD_SET_GREEN: set green color of the LED.
*   - CMD_SET_BLUE:  set blue color of the LED.
*   - CMD_SET_OFF:   turn off the LED.
*
* Return:
*  Returns status of command execution. There are two statuses
*  - STS_CMD_DONE: command is executed successfully.
*  - STS_CMD_FAIL: unknown command
*
*******************************************************************************/
static uint8 ExecuteCommand(uint32 cmd)
{
    uint8 status;

    status = STS_CMD_DONE;

    /* Execute received command */
    switch (cmd)
    {
        case CMD_SET_RED:
            RGB_LED_ON_RED;
            break;

        case CMD_SET_GREEN:
            RGB_LED_ON_GREEN;
            break;

        case CMD_SET_BLUE:
            RGB_LED_ON_BLUE;
            break;

        case CMD_SET_OFF:
            RGB_LED_OFF;
            break;

        default:
            status = STS_CMD_FAIL;
        break;
    }

    return (status);
}


/* [] END OF FILE */
