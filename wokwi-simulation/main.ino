#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <semphr.h>
#include <Wire.h>
#include <Servo.h>

/* --------------------------------------------------*/
/* ----------------- Prototipo Tarefas --------------*/
/* --------------------------------------------------*/

static void task_tanque_frio( void *pvParameters );
static void task_tanque_quente( void *pvParameters );
static void task_aquecimento( void *pvParameters );

/* --------------------------------------------------*/
/* --------------------- Semáforos ------------------*/
/* --------------------------------------------------*/

/* 
Os semaforos vão bloquear as tasks.
*/

static SemaphoreHandle_t tanque_frio;
static SemaphoreHandle_t tanque_quente;
static SemaphoreHandle_t temperatura;
static SemaphoreHandle_t aquecimento; 

/* --------------------------------------------------*/
/* --------------------- Variaveis ------------------*/
/* --------------------------------------------------*/

int limites_quente;
int shared_temperatura = 23;

void setup() {
  Serial.begin(9600);
  
  while (!Serial) {
    ; /* Somente vai em frente quando a serial estiver pronta para funcionar */
  }

  /* Criação dos semaforos */
  tanque_frio = xSemaphoreCreateBinary();
  tanque_quente = xSemaphoreCreateBinary();
  temperatura = xSemaphoreCreateBinary();
  aquecimento = xSemaphoreCreateBinary();


  xTaskCreate( task_tanque_frio,                   
               ( const char * ) "Tanque frio",
               100,                                  
               NULL,                                 
               tskIDLE_PRIORITY + 1,                 
               NULL );
                               
  
  xTaskCreate( task_tanque_quente,                   
               ( const char * ) "Tanque quente",
               100,                                    
               NULL,                                   
               tskIDLE_PRIORITY + 1,                   
               NULL );                                 

  xTaskCreate( task_aquecimento,                    
               ( const char * ) "Tanque quente", 
               100,                                   
               NULL,                                  
               tskIDLE_PRIORITY + 1,                  
               NULL );   

  xSemaphoreGive(tanque_frio);
  xSemaphoreGive(aquecimento);

  /* Inicialização do escalonador */
  vTaskStartScheduler();
  
  return ;
}

void loop()
{
  /* Tudo é executado nas tarefas. Há nada a ser feito aqui. */
}

/* --------------------------------------------------*/
/* ---------------------- Tarefas -------------------*/
/* --------------------------------------------------*/


static void task_tanque_frio( void *pvParameters )
{
  Servo valvula_entra_fria;
  Servo valvula_sai_fria;
  int potPin = A0;
  int potValue;
  int limites;
  
  while (1)
  {
    valvula_sai_fria.attach(8);
    valvula_entra_fria.attach(9);
    potValue = analogRead(potPin);
    limites = map(potValue, 0, 1023, 0, 100);

    if(limites >= 90) {
      valvula_entra_fria.write(0);
    }
    
    xSemaphoreTake(tanque_frio, portMAX_DELAY );
    if (limites <= 20 | limites_quente >= 90) {
      valvula_sai_fria.write(0);
    } else {
      valvula_sai_fria.write(90);
    }
    xSemaphoreGive(tanque_quente);

    if (limites > 20 && limites < 90) {
      valvula_entra_fria.write(90);
    }
  }
}

static void task_tanque_quente( void *pvParameters )
{ 
  Servo valvula_sai_quente;
  int potPin1 = A1;
  int potValue1;

  while (1)
  { 
    potValue1 = analogRead(potPin1);
    valvula_sai_quente.attach(7);
    
    xSemaphoreTake(tanque_quente, portMAX_DELAY );
    limites_quente = map(potValue1, 0, 1023, 0, 100);
    xSemaphoreGive(tanque_frio); 
    
    xSemaphoreTake(temperatura, portMAX_DELAY );
    if(limites_quente <= 20 | shared_temperatura < 70) {
      valvula_sai_quente.write(90);
    } else {
      valvula_sai_quente.write(0);
    }

    if(limites_quente > 20 && shared_temperatura >= 70) {
      valvula_sai_quente.write(90);
    } else {
      valvula_sai_quente.write(0);
    }
    xSemaphoreGive(aquecimento);
  }
}

static void task_aquecimento( void *pvParameters )
{
  const int LED_PIN_RED = 13;
  const int LED_PIN_BLUE = 12;
  int potPin2 = A2;
  int potValue2;
  bool var_aquecimento;
  
  while (7)
  {  
    pinMode(LED_PIN_RED, OUTPUT);
    pinMode(LED_PIN_BLUE, OUTPUT);

    xSemaphoreTake(aquecimento, portMAX_DELAY );
    potValue2 = analogRead(potPin2);
    shared_temperatura = map(potValue2, 0, 1023, 0, 100);
    Serial.println("(AQ)Temperatura: ");
    Serial.println(shared_temperatura);
    xSemaphoreGive(temperatura);
    
    if(shared_temperatura >= 70) {  
      digitalWrite(LED_PIN_RED, LOW);
      digitalWrite(LED_PIN_BLUE, HIGH);
      var_aquecimento = false;
    } else {
      digitalWrite(LED_PIN_RED, HIGH);
      digitalWrite(LED_PIN_BLUE, LOW);
      var_aquecimento = true;
    }
  }
}