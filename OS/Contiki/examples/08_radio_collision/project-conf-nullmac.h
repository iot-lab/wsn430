#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_
/* radio 802.15.4 conf */

#undef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC nullmac_driver

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC nullrdc_driver

#endif /* PROJECT_CONF_H_ */

