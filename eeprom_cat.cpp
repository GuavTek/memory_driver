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
	maxAddr = conf.maxAddr;
}

uint8_t eeprom_cat_c::set_wrenable(bool enabled){
	if (com->Get_Status() == Idle){

		if (enabled){
			msgWren = 1;
			memHeader[0] = 0b00000110;
		} else {
			msgWren = 0;
			memHeader[0] = 0b00000100;
		}

		Set_SS(1);
		com->Transfer(&memHeader[0], 1, RxTx);
		return 1;
	}
	return 0;
}

uint8_t eeprom_cat_c::write_data(char* src, uint8_t section, uint32_t index){
	if (com->Get_Status() == com_state_e::Idle){
		msgHeader = 1;
		msgWrite = 1;
		memAddr = (index * sections[section].objectSize) + sections[section].offset;
		msgRem = sections[section].objectSize;
		if ((memAddr + msgRem) > maxAddr){
			// Out of memory
			return 0;
		}
		msgBuff = src;
		transfer();
		return 1;
	}
	return 0;
}

uint8_t eeprom_cat_c::get_status(){
	if (com->Get_Status() == com_state_e::Idle){
		msgStatus = 1;
		memHeader[0] = 0b0101;
		memHeader[1] = 0;
		memHeader[2] = 0;
		Set_SS(1);
		com->Transfer(&memHeader[0], 3, RxTx);
		return 1;
	}
	return 0;
}

uint8_t eeprom_cat_c::read_data(char* dest, uint8_t section, uint32_t index){
	if (com->Get_Status() == com_state_e::Idle){
		msgHeader = 1;
		msgWrite = 0;
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

uint8_t eeprom_cat_c::read_items(char* dest, uint8_t section, uint32_t index, uint32_t num){
	if (com->Get_Status() == com_state_e::Idle){
		msgHeader = 1;
		msgWrite = 0;
		memAddr = (index * sections[section].objectSize) + sections[section].offset;
		msgRem = sections[section].objectSize * num;
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
	if (msgStatus) {
		msgStatus = 0;
		wrBusy = memHeader[1] & 0b1;
	}
	if (msgHeader){
		Set_SS(0);
		if (wrBusy) {
			get_status();	// TODO: let other drivers use bus
		} else if (!msgWrite || msgWren){
			msgHeader = 0;
			msgWren = 0;	// wren resets after each write operation
			wrBusy = msgWrite;
			memHeader[0] = msgWrite ? 0b0010 : 0b0011;
			memHeader[1] = (memAddr >> 8) & 0xff;
			memHeader[2] = memAddr & 0xff;
			Set_SS(1);
			com->Transfer(&memHeader[0], 3, RxTx);
		} else {
			set_wrenable(1);
		}
	} else {
		uint8_t maxLen = 64 - (memAddr & (64-1));	// Bytes until page end
		msgHeader = 1;
		if (msgRem > maxLen){
			com->Transfer(msgBuff, maxLen, RxTx);
			msgRem -= maxLen;
			msgBuff = &msgBuff[maxLen];
			memAddr += maxLen;
		} else {
			// Final data being sent
			com->Transfer(msgBuff, msgRem, RxTx);
			msgRem = 0;
		}
	}
}

void eeprom_cat_c::com_cb(){
	if (msgRem <= 0){
		// Transaction done
		Set_SS(0);
		if (msgStatus) {
			msgStatus = 0;
			wrBusy = memHeader[1] & 0b1;
		}
		if (wrBusy) {
			// Check status register until it is ready for write operations
			// TODO: this does not need to hog the bus, let other drivers use it
			get_status();
		} else {
			complete_cb();
		}
	} else {
		// Continue transfer
		transfer();
	}
}
