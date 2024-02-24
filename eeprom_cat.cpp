/*
 * eeprom_cat.cpp
 *
 * Created: 04/02/2024 19:05:31
 *  Author: GuavTek
 */ 

#include "eeprom_cat.h"
#include <malloc.h>

void eeprom_cat_c::init(const eeprom_cat_conf_t conf, const eeprom_cat_section_t sec[], uint8_t num_section){
	sections = (eeprom_cat_section_t*) malloc(num_section * sizeof(eeprom_cat_section_t));
	for (uint8_t i = 0; i < num_section; i++){
		sections[i] = sec[i];
	}
	comSlaveNum = conf.comSlaveNum;
	maxAddr = conf.maxAddr;
	com->Set_Slave_Callback(comSlaveNum, this);

}

uint8_t eeprom_cat_c::set_wrenable(bool enabled){
	if (com->Get_Status() == Idle){
		
		if (enabled){
			memHeader[0] = 0b00000110;
		} else {
			memHeader[0] = 0b00000100;
		}
		
		com->Select_Slave(comSlaveNum);
		com->Transfer(&memHeader[0], 1, RxTx);
		return 1;
	}
	return 0;
}

uint8_t eeprom_cat_c::write_data(char* src, uint8_t section, uint32_t index){
	if (com->Get_Status() == com_state_e::Idle){
		msgHeader = 0;
		msgWrite = 1;
		msgWren = 0;
		memAddr = (index * sections[section].objectSize) + sections[section].offset;
		msgRem = sections[section].objectSize;
		if ((memAddr + msgRem) > maxAddr){
			// Out of memory
			return 0;
		}
		msgBuff = src;
		set_wrenable(1);
		return 1;
	}
	return 0;
}

uint8_t eeprom_cat_c::read_data(char* dest, uint8_t section, uint32_t index){
	if (com->Get_Status() == com_state_e::Idle){
		msgHeader = 1;
		msgWrite = 0;
		msgWren = 1;
		memAddr = (index * sections[section].objectSize) + sections[section].offset;
		msgRem = sections[section].objectSize;
		if ((memAddr + msgRem) > maxAddr){
			// Out of memory
			return 0;
		}
		msgBuff = dest;
		transfer();
		return 1;
	}
	return 0;
}

void eeprom_cat_c::transfer(){
	if (msgHeader){
		com->Select_Slave(-1);
		msgHeader = 0;
		memHeader[0] = msgWrite ? 0b0010 : 0b0011;
		memHeader[1] = (memAddr >> 8) & 0xff;
		memHeader[2] = memAddr & 0xff;
		com->Select_Slave(comSlaveNum);
		com->Transfer(&memHeader[0], 3, RxTx);
	} else {
		uint8_t maxLen = 64 - (memAddr & (64-1));	// Bytes until page end
		msgHeader = 1;
		if (msgRem > maxLen){
			com->Transfer(msgBuff, maxLen, RxTx);
			msgRem -= maxLen;
			msgBuff = &msgBuff[maxLen];
			memAddr += maxLen;
			msgWren = !msgWrite;
		} else {
			// Final data being sent
			com->Transfer(msgBuff, msgRem, RxTx);
			msgRem = 0;
		}
	}
}

void eeprom_cat_c::com_cb(){
	if (msgRem > 0){
		// Transaction done
		com->Select_Slave(-1);
		complete_cb();
	} else if (!msgWren){
		// Send WREN command
		com->Select_Slave(-1);
		msgWren = 1;
		set_wrenable(1);
	} else {
		// Continue transfer
		transfer();
	}
}
