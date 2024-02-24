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
	uint8_t comSlaveNum;
	uint32_t maxAddr;
};

class eeprom_cat_c : public com_driver_c {
	public:
	void init(const eeprom_cat_conf_t conf, const eeprom_cat_section_t sec[], uint8_t num_section);
	uint8_t set_wrenable(bool enabled);
	uint8_t write_data(char* src, uint8_t section, uint32_t index);
	uint8_t read_data(char* dest, uint8_t section, uint32_t index);
	inline void set_callback(void (*cb)()) {complete_cb = cb;}
	void com_cb();
	eeprom_cat_c(communication_base_c* const comInstance) {com = comInstance;};
	~eeprom_cat_c() {};
	protected:
	communication_base_c* com;
	uint8_t comSlaveNum;
	void transfer();
	uint32_t maxAddr;
	void (*complete_cb)();
	bool msgHeader;
	bool msgWrite;
	bool msgWren;
	char memHeader[3];
	uint16_t memAddr;
	char* msgBuff;
	uint16_t msgRem;
	eeprom_cat_section_t* sections;
};


#endif /* EEPROM_CAT_H_ */