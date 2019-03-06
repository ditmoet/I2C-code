//Defines
#define F_CPU 16000000			//CPU frequency
#define SCL 100000				//TWI clock rate

#define SLAVE_ADDRESS 3

#define SEND_DATA_SIZE 5
#define RECEIVE_DATA_SIZE 5

//Includes
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

//Forward Declaration
	//Initializers
void init_io();						//Set DDR registers
void init_interrupt();				//Enable interrupt
void init_USART();					//Initialize USART operation
void init_TWI();					//Initialize the TWI

	//TWI functions
ISR(TWI_vect);								//Handle TWI functions based on TWI status

	//USART functions
void writeChar(char character);				//Write a character over the serial connection
void writeString(char* charArray);			//Write an array of characters over the serial connection
void writeInt(int16_t integer);				//Write an integer over the serial connection

//Global variables
uint8_t sendData[SEND_DATA_SIZE];
uint8_t receivedData[RECEIVE_DATA_SIZE];

//Main
int main(){
	//Initializers
	init_io();
	init_interrupt();
	init_USART();
	init_TWI();
	
	while(1){
		//Main loop code here
	}
}

//Function definitions
//Initializers
void init_io(){
	PORTC = 0b00110000;		//Enable pull-up resistor on SDA and SCL for TWI
}
void init_interrupt(){
	sei();
}
void init_USART(){
	UCSR0B = (1 << TXEN0);					//Enable USART Transmitter in register B
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);	//Set the package data size to 8 bit
	UBRR0 = 103;							//Baudrate set to 9600
}
void init_TWI(){
	TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWEA);		//Enable TWI; Enable Interrupt; Enable acknowledgment bit
	TWBR = ((F_CPU / SCL) - 16) / 2;					//Set SCL to 100kHz
	TWAR = (SLAVE_ADDRESS << 1);						//Set slave address (general call disabled)
}

//TWI functions
ISR(TWI_vect){
	static int byteNumber = 0;					//Count the bytes that are send or received
	
	switch(TWSR){
		case 0x60:									//0x60 Own SLA+W has been received; ACK has been returned
			byteNumber = 0;								//Reset the byte counter to start receiving data
		break;
		
		case 0x80:									//0x80 Previously addressed with own SLA+W; data has been received; ACK has been returned
			if(byteNumber < RECEIVE_DATA_SIZE){			//If the array has space left
				receivedData[byteNumber] = TWDR;			//Read the data and save it
				byteNumber++;								//Increase counter for the next byte
			}else{										//If the array is full
				writeString("\n\n\rError: To much data received!!\n\n\r");	//Send an error to the monitor
			}
		break;
		
		case 0xA8:									//0xA8 Own SLA+R has been received; ACK has been returned
			byteNumber = 0;								//Reset the counter to start sending data
			TWDR = sendData[byteNumber];				//Write the first byte to the data register for the TWI to send to the master
		break;
		
		case 0xB8:									//0xB8 Data byte in TWDR has been transmitted; ACK has been received
			byteNumber++;
			TWDR = sendData[byteNumber];				//Write data to the data register for the TWI to send to the master
		break;
	}
	
	TWCR |= (1 << TWINT);						//Continue TWI operations and execute given tasks
}

//USART functions
void writeChar(char character) {
	while(~UCSR0A & (1 << UDRE0));		//Wait until the USART data register is flagged empty
	UDR0 = character;					//Set the current char value in the data register to transmit
}
void writeString(char* charArray) {
	for(uint8_t i = 0; charArray[i] != 0; i++) {	//Keep looping through the char array and send the characters until there is no more left
		writeChar(charArray[i]);					//Write the character of position i from the array
	}
}
void writeInt(int16_t integer) {
	char buffer[16];							//Create a buffer to store the string returned by itoa
	itoa(integer,buffer,10);					//Convert the integer to a string and store it in the buffer, it is converted tot a decimal system
	writeString(buffer);						//Write the string to the serial
}

