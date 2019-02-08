#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "i2c.h"

#define I2C_ADDR 0x62

static int piwatcher_open(void)
{
    int dev;

    dev = i2c_open_dev(1);

    if (dev<0) {
        fprintf(stderr,"Failed to open i2c device 1\n");
        return dev;
    }

    return dev;
}

static int piwatcher_read_register(uint8_t reg)
{
    int i2c = piwatcher_open();
    if (i2c<0)
        return i2c;

    int res = i2c_read_byte_data(i2c, I2C_ADDR, reg);
    close(i2c);

    if (res<0) {
        fprintf(stderr,"Failed to read i2c data from register 0x%02x\n", reg);
    }
    return res;
}

static int piwatcher_write_register(uint8_t reg, uint8_t value)
{
    int i2c = piwatcher_open();
    if (i2c<0)
        return i2c;

    int res = i2c_write_byte_data(i2c, I2C_ADDR, reg, value);
    close(i2c);

    if (res<0) {
        fprintf(stderr,"Failed to write i2c data to register 0x%02x\n", reg);
    }
    return res;
}

static int piwatcher_read_block(uint8_t reg, uint8_t *buf, uint8_t count)
{
    int i2c = piwatcher_open();
    if (i2c<0)
        return i2c;

    int res = i2c_read_block(i2c, I2C_ADDR, reg, buf, count);
    close(i2c);

    if (res<0) {
        fprintf(stderr,"Failed to read i2c block from register 0x%02x\n", reg);
    }
    return res;
}

static int piwatcher_write_block(uint8_t reg, uint8_t *buf, uint8_t count)
{
    int i2c = piwatcher_open();
    if (i2c<0)
        return i2c;

    int res = i2c_write_block(i2c, I2C_ADDR, reg, buf, count);
    close(i2c);

    if (res<0) {
        fprintf(stderr,"Failed to write i2c block to register 0x%02x\n", reg);
    }
    return res;
}

static const char *status_to_string(uint8_t status)
{
    static char buf[100];
    
    buf[0]=0;

    if ((status & 0x80)!=0) {
        strcat(buf,"button_pressed ");
    }
    if ((status & 0x40)!=0) {
        strcat(buf,"timer_rebooted ");
    }
    if ((status & 0x20)!=0) {
        strcat(buf,"button_rebooted ");
    }
    return buf;
}

static const char *time_to_string(uint32_t wd)
{
    static char buf[32];

    if (wd==0)
        return "disabled";
    sprintf(buf, "%u", wd);
    return buf;
}

static int as_seconds(const char *s)
{
    char *next;

    if (strcmp(s,"disable")==0)
        return 0;

    long val = strtol(s, &next, 0);
    if (next==s)
        return -1;
    if (*next!=0)
        return -2;
    return val;
}   

/******/

static int cmd_status(int argc, char **argv)
{
    int res = piwatcher_read_register(0);

    if (res<0) {
        fprintf(stdout,"ERR\n");
        return res;
    } 
    fprintf(stdout,"OK\t0x%02x\t%s\n", res&0xFF, status_to_string(res));
    return 0;
}

int cmd_reset(int argc, char **argv)
{
    int res = piwatcher_write_register(0, 0xFF);

    if (res<0) {
        fprintf(stdout, "ERR\n");
        return res;
    } 
    fprintf(stdout,"OK\n");
    return 0;
}

int cmd_watch(int argc, char **argv)
{
    if (argc==0)
    {
        int res = piwatcher_read_register(1);
        
        if (res<0) {
            fprintf(stdout,"ERR\n");
            return res;
        } 
        fprintf(stdout,"OK\t%s\n", time_to_string(res));
    }
    else 
    {
        int wdd;
        if ((wdd = as_seconds(argv[0]))<0)
        {
            fprintf(stderr,"Syntax error in watchdog timeout value '%s'.\n", argv[0]);
            fprintf(stdout,"ERR\n");
            return wdd;
        }
        if (wdd>255)
        {
            fprintf(stderr,"Watchdog value cannot exceed 255.\n");
            fprintf(stdout,"ERR\n");
            return -1;
        }
        int res = piwatcher_write_register(1, wdd&0xFF);
        if (res<0) {
            fprintf(stdout,"ERR\n");
            return res;
        }
        fprintf(stdout,"OK\n");
    }
    return 0;
}

int cmd_dump(int argc, char **argv)
{
    uint8_t block[32];
    int res = piwatcher_read_block(0,block,8);
    
    if (res<0) {
        fprintf(stdout, "ERR\n");
        return res;
    } 
    fprintf(stdout,"OK\t");
    for (int i=0;i<8;i++)
        fprintf(stdout,"%02x ", block[i]);
    fprintf(stdout, "\n");
    return 0;
}

int cmd_ticks(int argc, char **argv)
{   
    uint8_t block[32];
    int res = piwatcher_read_block(4,block,4);

    if (res<0) {
        fprintf(stdout, "ERR\n");
        return res;
    }
    uint32_t ticks = ((uint32_t)block[0]) 
        | ((uint32_t)block[1]<<8) 
        | ((uint32_t)block[2]<<16) 
        | ((uint32_t)block[3]<<24);

    fprintf(stdout,"OK\t%u\n", ticks);
    return 0;
}   

int cmd_wake(int argc, char **argv)
{
    char block[2];

    if (argc==0)
    {
        int res = piwatcher_read_block(2,block,2);

        if (res<0) {
            fprintf(stdout,"ERR\n");
            return res;
        }
        uint32_t seconds = ((uint32_t)block[0]<<1) | ((uint32_t)block[1]<<9);
        fprintf(stdout,"OK\t%s\n", time_to_string(seconds));
    }
    else
    {
        uint32_t wku;
        if ((wku = as_seconds(argv[0]))<0)
        {
            fprintf(stderr,"Syntax error in wakeup timeout value '%s'.", argv[0]);
            fprintf(stdout,"ERR\n");
            return wku;
        }
        if (wku>131070) {
            fprintf(stderr,"Wakeup value cannot exceed 131070.\n");
            fprintf(stdout,"ERR\n");
            return -1; 
        }
        if ((wku&1)!=0) {
            fprintf(stderr,"Warning: wakeup value will be rounded to %u.\n", wku+1);
        }
        wku = (wku+1)>>1;

        block[0] = wku & 0xFF;
        block[1] = wku >> 8;
            
        int res = piwatcher_write_block(2, block, 2);
        if (res<0) {
            fprintf(stdout,"ERR\n");
            return res;
        }
        fprintf(stdout,"OK\n");
    }
    return 0;
}       


struct command_desc {
    const char *command;
    int (*run)(int, char **);
    const char *help_text;
};

int cmd_help(int argc, char **argv);

#define COUNT_COMMANDS 7
struct command_desc commands[COUNT_COMMANDS] = {
    { "ticks", cmd_ticks, "print the current number of clock ticks," },
    { "dump", cmd_dump, "dump the registers in hexadecimal." },
    { "help", cmd_help, "print this help" },
    { "reset", cmd_reset, "clear the status register" },
    { "status", cmd_status, "print the status register" },
    { "wake", cmd_wake, "show or set the current wake delay in seconds." },
    { "watch", cmd_watch, "show or set the current watchdog interval in seconds." },
};

int cmd_help(int argc, char **argv)
{
    fprintf(stdout,"piwatcher <command> [argument]\n");
    fprintf(stdout,"Where command is:\n");
    for (int i=0;i<COUNT_COMMANDS;i++)
        fprintf(stdout, "\t%s: %s\n", commands[i].command, commands[i].help_text);
    fprintf(stdout,"For more info: https://www.omzlo.com/the-piwatcher\n");
}

int main(int argc, char **argv)
{
    //int optind = 1;
    
    if (argc<=1) {
        fprintf(stderr,"Missing command\n");
        fprintf(stderr,"Type '%s help' for usage\n", argv[0]);
        exit(-2);
    }

    for (int i=0; i<COUNT_COMMANDS; i++) {
        if (strcmp(commands[i].command, argv[1])==0)
        {
            return commands[i].run(argc-2, argv+2); 
        }
    }
    
    fprintf(stderr,"Unrecognized command '%s'\n", argv[1]);
    fprintf(stderr,"Type '%s help' for usage\n", argv[0]);
}
