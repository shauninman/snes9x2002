#ifndef CONFIG_H__
#define CONFIG_H__

typedef struct {
	int32_t fullscreen;
	/* For input remapping */
	uint32_t config_buttons[6][19];
} t_config;
extern t_config option;

#endif
