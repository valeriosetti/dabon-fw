#ifndef _SI468X_H_
#define _SI468X_H_

// Functions' return codes
#define SI468X_SUCCESS		0L
#define SI468X_ERROR		-1L

// Public functions
void Si468x_init(void);
int Si468x_get_digital_service_list(void);

//

#endif //_SI468X_H_
