#ifndef PROTOCOL_H
#define PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Initialisation de la communication AirPort sur USART6. */
void Protocol_Init(void);

/* À appeler régulièrement dans la boucle principale. */
void Protocol_Task(void);

#ifdef __cplusplus
}
#endif

#endif /* PROTOCOL_H */
