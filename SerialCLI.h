#ifndef SERIALCLI_H_INCLUDED
#define SERIALCLI_H_INCLUDED
/*
 * SerialCLI.h
 *
 * Author:   Hiromasa Ihara (taisyo)
 * Created:  2016-02-18
 */

#include <Arduino.h>

#include <EEPROM.h>
#include <Ethernet.h>
#include <SPI.h>

#include <avr/wdt.h>

#define SERIALCLI_VERSION 0.01
#define SERIALCLI_LINE_BUF_SIZE 128
#define SERIALCLI_MAX_ENTRY_SIZE 16
#define SERIALCLI_NORMAL_MODE 0
#define SERIALCLI_CONF_MODE 1

class Entry
{
  public:
    String name;
    String default_value;
    String description;
    String value;
    Entry(String name, String default_value, String description){
      this->name = name;
      this->default_value = default_value;
      this->description = description;
      this->value = "";
    }
    virtual int parse(String value) = 0;
    virtual int serialize(int eeprom_index) = 0;
    virtual int deserialize(int eeprom_index) = 0;
};

class StringEntry : public Entry
{
  public:
    StringEntry(String name, String default_value, String description)
      : Entry(name, default_value, description)
    {
    }

    int parse(String value){
      return 0;
    }

    int serialize(int eeprom_index){
      int i;
      for (i = 0; i < value.length(); i++) {
        EEPROM.write(eeprom_index+i, value.charAt(i));
      }
      EEPROM.write(eeprom_index+i, '\0');

      return value.length()+1;
    }

    int deserialize(int eeprom_index){
      char c;
      value = "";
      while(1){
        c = EEPROM.read(eeprom_index++);
        if(c == '\0'){
          break;
        }
        value += c;
      }

      return value.length()+1;
    }

    String get_val(){
      return value;
    }
};

class BoolEntry : public Entry
{
  public:
    BoolEntry(String name, String default_value, String description)
      : Entry(name, default_value, description)
    {
    }

    int parse(String value){
      return 0;
    }

    int serialize(int eeprom_index){
      if(value.equals("true")){
        EEPROM.write(eeprom_index, 1);
      }else if(value.equals("false")){
        EEPROM.write(eeprom_index, 1);
      }else{
        return 0;
      }

      return 1;
    }

    int deserialize(int eeprom_index){
      int v = EEPROM.read(eeprom_index);
      if(v==1){
        value = "true";
      }else if(v==0){
        value = "false";
      }else{
        return 0;
      }

      return 1;
    }

    bool get_val(){
      if(value.equals("true")){
        return 1;
      }else{
        return 0;
      }
    }
};

class IntegerEntry: public Entry
{
  public:
    IntegerEntry(String name, String default_value, String description)
      : Entry(name, default_value, description)
    {
    }

    int parse(String value){
      int ret;
      ret = sscanf(value.c_str(), "%*d");
      if(ret != 1){return -1;}
      return 0;
    }

    int serialize(int eeprom_index){
      uint32_t integer;
      sscanf(value.c_str(), "%d", &integer);

      EEPROM.write(eeprom_index+0, (integer>>0) &0xff);
      EEPROM.write(eeprom_index+1, (integer>>8) &0xff);
      EEPROM.write(eeprom_index+2, (integer>>16)&0xff);
      EEPROM.write(eeprom_index+3, (integer>>24)&0xff);

      return 4;
    }
    int deserialize(int eeprom_index){
      uint32_t integer;
      integer  = EEPROM.read(eeprom_index+3); integer <<= 8;
      integer += EEPROM.read(eeprom_index+2); integer <<= 8;
      integer += EEPROM.read(eeprom_index+1); integer <<= 8;
      integer += EEPROM.read(eeprom_index+0);
      value = integer;
      return 4;
    }
    uint32_t get_val(){
      uint32_t integer;
      sscanf(value.c_str(), "%d", &integer);
      return integer;
    }
};

class IPAddressEntry: public Entry
{
  public:
    IPAddressEntry(String name, String default_value, String description)
      : Entry(name, default_value, description)
    {
    }

    int parse(String val){
      int first_octet, second_octet, third_octet, fourth_octet;
      int ret;

      ret = sscanf(value.c_str(), "%d.%d.%d.%d", &first_octet, &second_octet, &third_octet, &fourth_octet);
      if(ret != 4){return -1;}
      return 0;
    }

    int serialize(int eeprom_index){
      int first_octet, second_octet, third_octet, fourth_octet;
      sscanf(value.c_str(), "%d.%d.%d.%d", &first_octet, &second_octet, &third_octet, &fourth_octet);

      EEPROM.write(eeprom_index+0, first_octet);
      EEPROM.write(eeprom_index+1, second_octet);
      EEPROM.write(eeprom_index+2, third_octet);
      EEPROM.write(eeprom_index+3, fourth_octet);

      return 4;
    }
    int deserialize(int eeprom_index){
      value = "";
      
      value += String(EEPROM.read(eeprom_index+0))+String('.');
      value += String(EEPROM.read(eeprom_index+1))+String('.');
      value += String(EEPROM.read(eeprom_index+2))+String('.');
      value += String(EEPROM.read(eeprom_index+3));
      return 4;
    }

    IPAddress get_val(){
      int first_octet, second_octet, third_octet, fourth_octet;
      sscanf(value.c_str(), "%d.%d.%d.%d", &first_octet, &second_octet, &third_octet, &fourth_octet);

      return IPAddress(first_octet, second_octet, third_octet, fourth_octet);
    }
};

class MacEntry: public Entry
{
  private:
    byte mac[6];

  public:
    MacEntry(String name, String default_value, String description)
      : Entry(name, default_value, description)
    {
    }

    int parse(String val){
      int mac[6];
      int ret;

      ret = sscanf(value.c_str(), "%x:%x:%x:%x:%x:%x",
          &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
      if(ret != 6){return -1;}
      return 0;
    }

    int serialize(int eeprom_index){
      int mac[6];

      sscanf(value.c_str(), "%x:%x:%x:%x:%x:%x",
          &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

      for (int i = 0; i < 6; i++) {
        EEPROM.write(eeprom_index+i, mac[i]);
      }

      return 6;
    }
    int deserialize(int eeprom_index){
      char buf[32];
      byte mac[6];

      for (int i = 0; i < 6; i++) {
        mac[i] = EEPROM.read(eeprom_index+i);
      }

      sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

      value = buf;
      
      return 6;
    }

    byte *get_val(){
      sscanf(value.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
          &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

      return mac;
    }
};

class SerialCLI
{
  private:
    String line_buf;
    Entry *entries[SERIALCLI_MAX_ENTRY_SIZE];
    int entry_size;
    HardwareSerial *serial;
    int state;
    struct extcmd {
      String cmdname;
      String help;
      void (*func)(void);
    } extcmds[SERIALCLI_MAX_ENTRY_SIZE];
    int extcmds_size;

  public:
    SerialCLI(HardwareSerial &serial) : serial(&serial){
      entry_size = 0;
      state = SERIALCLI_NORMAL_MODE;
    };

    ~SerialCLI(){
    };

    void begin(int baudrate, String initialize_message=""){
      load();

      serial->begin(baudrate);

      serial->println(initialize_message);
      serial->print("SerialCLI Ver.");
      serial->println(SERIALCLI_VERSION);
      serial->println("current setting");
      show();
      serial->println("setting description (and default value)");
      help();
      serial->println("commands");
      serial->println("    help, show, conf, exit, load [default], save,");
      serial->print("    ");
      for (int i = 0; i < extcmds_size; i++) {
        serial->print(extcmds[i].cmdname);
        serial->print(", ");
      }
      serial->println();
      serial->print(">");
    }


    void add_entry(Entry *entry)
    {
      entries[entry_size] = entry;
      entry_size++;

      serial->println(entry->default_value);
    }

    Entry *get_entry(String name){
      for (int i = 0; i < entry_size; i++) {
        if(entries[i]->name.equals(name)){
          return entries[i];
        }
      }
      return NULL;
    }

    void add_command(String cmdname, void (*func)(void)){
      extcmds[extcmds_size].cmdname = cmdname;
      extcmds[extcmds_size].func = func;
      extcmds_size++;
    }

    void help(void){
      for (int i = 0; i < entry_size; i++) {
        serial->print("    ");
        serial->print(entries[i]->name);
        serial->print("... ");
        serial->print(entries[i]->description);
        serial->print("(default ");
        serial->print(entries[i]->default_value);
        serial->println(")");
      }
    }

    void reboot(void){
      wdt_enable(WDTO_15MS);
      wdt_reset();
      while(1)
        ;
    }

    void show(void){
      for (int i = 0; i < entry_size; i++) {
        serial->print("    ");
        serial->print(entries[i]->name);
        serial->print("=");
        serial->println(entries[i]->value);
      }
    }

    void save(void){
      serial->print("saving...");
      int eeprom_index = 0;
      for (int i = 0; i < entry_size; i++) {
        int size;
        size = entries[i]->serialize(eeprom_index);
        eeprom_index += size;
      }
      serial->println("done.");
    }

    void load(void){
      serial->print("loading...");
      int eeprom_index = 0;
      for (int i = 0; i < entry_size; i++) {
        int size;
        size = entries[i]->deserialize(eeprom_index);
        if(size == 0){
          serial->println("failed.");
          load_default();
          return;
        }
        eeprom_index += size;
      }
      serial->println("done.");
    }

    void load_default(void){
      serial->print("loading(default)...");
      for (int i = 0; i < entry_size; i++) {
        entries[i]->value = entries[i]->default_value;
      }
      serial->println("done.");
    }

    void set(void){
      int name_len = line_buf.indexOf("=");
      int i;
      for (i = 0; i < entry_size; i++) {
        if(line_buf.substring(0, name_len).equals(entries[i]->name)){
          if(entries[i]->parse(line_buf.substring(name_len+1)) == 0){
            entries[i]->value = line_buf.substring(name_len+1);
          }else{
            serial->println("invalid format");
          }
          return;
        }
      }

      serial->print("command not found:");
      serial->println(line_buf.substring(0, name_len));
    }

    void process_command(void)
    {
      if(line_buf.equals("help")){
        help();
      }else if(line_buf.equals("reboot") || line_buf.equals("restart")){
        reboot();
      }else if(line_buf.equals("show")){
        show();
      }else if(line_buf.equals("conf")){
        if(state == SERIALCLI_NORMAL_MODE){
          serial->println("enter conf mode");
          state = SERIALCLI_CONF_MODE;
        }else{
          serial->println("already conf mode");
        }
      }else if(line_buf.equals("exit")){
        if(state == SERIALCLI_CONF_MODE){
          serial->println("enter normal mode");
          state = SERIALCLI_NORMAL_MODE;
        }else{
          serial->println("normal mode");
        }
      }else if(line_buf.equals("save")){
        if(state == SERIALCLI_CONF_MODE){
          save();
        }else{
          serial->println("please switch conf mode (ex. >conf)");
        }
      }else if(line_buf.equals("load")){
        if(state == SERIALCLI_CONF_MODE){
          load();
        }else{
          serial->println("please switch conf mode (ex. >conf)");
        }
      }else if(line_buf.equals("load default")){
        if(state == SERIALCLI_CONF_MODE){
          load_default();
        }else{
          serial->println("please switch conf mode (ex. >conf)");
        }
      }else if(line_buf.indexOf("=") != -1){
        if(state == SERIALCLI_CONF_MODE){
          set();
        }else{
          serial->println("please switch conf mode (ex. >conf)");
        }
      }else if(line_buf.equals("")){
        // do nothing
      }else{
        int i;
        for (i = 0; i < extcmds_size; i++) {
          serial->println(extcmds[i].cmdname);
          if (line_buf.equals(extcmds[i].cmdname)) {
            extcmds[i].func();
            break;
          }
        }

        if(i == extcmds_size){
          serial->print("unknown command:");
          serial->println(line_buf);
        }
      }
    }

    void process(void){
      int prompt_flag;
      while(serial->available()){
        prompt_flag = false;
        char c = serial->read();
        serial->print(c);
        if(c == '\n'){
          process_command();

          line_buf = "";
          prompt_flag = true;
        }else if(c == '\r'){
          continue;
        }else if(line_buf.length() == SERIALCLI_LINE_BUF_SIZE-1){
          serial->print("too long!");
          while(serial->available()){
            char c = serial->read();
            serial->print(c);
          }

          line_buf = "";
          prompt_flag = true;
        }else{
          line_buf += c;
        }
        if(prompt_flag){
          if(state == SERIALCLI_NORMAL_MODE){
            serial->print(">");
          }else{
            serial->print("#");
          }
        }
      }
    }
};

#endif   /* SERIALCLI_H_INCLUDED */
