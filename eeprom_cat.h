/*
 * eeprom_cat.h
 *
 * Created: 04/02/2024 19:06:12
 *  Author: GuavTek
 */

// A driver for CAT25640 EEPROM or similar memory chips

#ifndef EEPROM_CAT_H_
#define EEPROM_CAT_H_

#include "communication_base.h"

struct eeprom_cat_section_t {
	uint32_t offset;	// Start address of section
	uint32_t objectSize;		// Size of objects in the section
};

struct eeprom_cat_conf_t {
	uint32_t maxAddr;
};

class eeprom_cat_c : public com_driver_c {
	public:
		void init(const eeprom_cat_conf_t conf, const eeprom_cat_section_t sec[], uint8_t num_section);
		uint8_t set_wrenable(bool enabled);
		uint8_t write_data(char* src, uint8_t section, uint32_t index);
		uint8_t read_data(char* dest, uint8_t section, uint32_t index);
		uint8_t read_items(char* dest, uint8_t section, uint32_t index, uint32_t num);
		uint8_t get_status();
		inline uint8_t is_busy() {return (com->Get_Status() != Idle) || wrBusy;}
		inline void set_callback(void (*cb)()) {complete_cb = cb;}
		void com_cb();
		using com_driver_c::com_driver_c;
	protected:
		void transfer();
		uint32_t maxAddr;
		void (*complete_cb)();
		bool msgHeader;
		bool msgWrite;
		bool msgWren;
		bool msgStatus;
		bool wrBusy;
		char memHeader[3];
		uint16_t memAddr;
		char* msgBuff;
		int16_t msgRem;
		eeprom_cat_section_t* sections;
};


#endif /* EEPROM_CAT_H_ */