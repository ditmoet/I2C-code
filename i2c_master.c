//Defines
#define F_CPU 16000000			//CPU frequency
#define SCL 100000				//TWI clock rate

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
ISR(TWI_vect);									//Handle TWI functions based on TWI status

void writeToSlave(uint8_t address, uint8_t* data);		//Send all values in data array to slave address
void readFromSlave(uint8_t address);					//Read data send by the slave

void TWIWrite(uint8_t byte);					//Write a single byte over the TWI
void TWIWaitUntilReady();						//Wait until the TWI has finished it's current job
void TWICheckCode(uint8_t code);				//Compare the given code to the code in the register and show error on mismatch
void TWI_Start();								//Start the TWI and send a start signal
void TWI_Stop();								//Start the TWI and send a stop signal
void TWI_ACK();									//Continue TWI operation with the acknowledgment bit enabled
void TWI_NACK();								//Continue TWI operation without ACK bit OR send data byte when TWDR contains data

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
	TWCR = (1 << TWEN) | (1 << TWIE);		//Enable TWI and Enable Interrupt
	TWBR = ((F_CPU / SCL) - 16) / 2;		//Set SCL to 100kHz
}

//TWI functions
ISR(TWI_vect){
	static int byteNumber = 0;					//Count the bytes received from the slave
	
	switch(TWSR){
		case 0x40:								//0x40 SLA+R has been transmitted; ACK has been received
			byteNumber = 0;							//Reset byte counter
			TWI_ACK();								//Continue TWI with acknowledgment enabled
		break;
		
		case 0x50:								//0x50 Data byte has been received; ACK has been returned
			receivedData[byteNumber] = TWDR;		//Read the data register and save the data
			byteNumber++;							//Increase the counter for the next byte
			if(byteNumber < RECEIVE_DATA_SIZE - 1){	//If the next byte is NOT the last
				TWI_ACK();								//Continue with ACK
			}else{									//Else
				TWI_NACK();								//Continue and send NACK after the next byte to stop the slave
			}
		break;
		
		case 0x58:								//0x58 Data byte has been received; NOT ACK has been returned
			receivedData[byteNumber] = TWDR;		//Read the last byte
			TWI_Stop();								//And stop the TWI connection
		break;
	}
}

void writeToSlave(uint8_t address, uint8_t* data){
	
	TWI_Start();								//Send a start signal to claim the data bus
	TWICheckCode(0x08);							//0x08 - A START condition has been transmitted
	
	TWIWrite(address << 1);						//Send an address over the data line to address a slave
	TWICheckCode(0x18);							//0x18 - SLA+W has been transmitted; ACK has been received
	
	for(int i = 0; i < SEND_DATA_SIZE; i++){	//Send all the bytes in the array one by one
		TWIWrite(data[i]);							//Send the current data byte to the slave
		TWICheckCode(0x28);							//0x28 - Data byte has been transmitted; ACK has been received
	}
	
	TWI_Stop();									//Stop the TWI connection
	
}
void readFromSlave(uint8_t address){
	
	TWI_Start();								//Send a start signal to claim the data bus
	TWICheckCode(0x08);							//0x08 - A START condition has been transmitted
	
	TWIWrite( (address << 1) + 1 );				//Send an address over the data line with a 'read' request to request data from a slave
	
	//The rest of the functions are handled by the ISR based on the different status codes
}

void TWIWrite(uint8_t byte){
	TWDR = byte;					//Set the data in the data register to be handled by the TWI
	TWI_NACK();						//Start the TWI send process, ACK is not needed because we are sending
	TWIWaitUntilReady();			//Wait until the TWI has finished its task to avoid outrunning the TWI
}
void TWIWaitUntilReady(){
	while (!(TWCR & (1 << TWINT)));	//Wait until the TWI has finished its task by listening to the INT bit
}
void TWICheckCode(uint8_t code){
	uint8_t TWIstatus = (TWSR & 0xF8);		//Read the status code by masking off the unused register bits in TWSR
	if(TWIstatus != code){
		writeString("\n\n\rERROR: Wrong status!\n\rTWI status: 0x");
		writeString( itoa( TWIstatus, buffer, 16) );
		writeString("\n\rExpected status: 0x");
		writeString( itoa( code, buffer, 16) );
		writeString("\n\n\r");
	}
}
void TWI_Start(){
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE)|(1<<TWSTA);	//Start TWI; Enable TWI; Enable TWI interrupt; Send start signal; 
	TWIWaitUntilReady();								//Wait until start is send;
}
void TWI_Stop(){
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE)|(1<<TWSTO);	//Start TWI; Enable TWI; Enable TWI interrupt; Send stop signal;
}
void TWI_ACK(){
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE)|(1<<TWEA);	//Continue TWI; Enable TWI; Enable TWI interrupt; Enable acknowledgment;
}
void TWI_NACK(){
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);				//Start/continue TWI; Enable TWI; Enable TWI interrupt;
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

