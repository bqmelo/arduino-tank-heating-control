#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <semphr.h>
#include <Wire.h>

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

bool shared_valvula_sai_fria = true;
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
  bool valvula_entra_fria = true;
  int lim_max;
  int lim_min;
  
  while (1)
  {
    lim_max = 89; // ler o sensor lim_max
    lim_min = 12; // ler o sensor lim_min

    if(lim_max >= 90) {
      Serial.println("(TF)Valvula EF: Fechada\n");
      valvula_entra_fria = false;
    }
    
    xSemaphoreTake(tanque_frio, portMAX_DELAY );
    if (lim_min <= 10) { // fecho a variavel compartilhada
      Serial.println("(TF)Valvula SF: Fechada\n");    
      shared_valvula_sai_fria = false;
    }
    xSemaphoreGive(tanque_quente);

    if (lim_min > 10 && lim_max < 90) {
      Serial.println("(TF)Valvula EF: Aberta\n");
      valvula_entra_fria = true;
    }
    vTaskDelay(13);
  }
}

static void task_tanque_quente( void *pvParameters )
{ 
  bool valvula_sai_quente = false;
  int lim_max;
  int lim_min;
  
  while (1)
  { 
    lim_min = 23; // ler o sensor min
    lim_max = 70; // ler o sensor max

    xSemaphoreTake(tanque_quente, portMAX_DELAY );
    if(lim_max >= 90) { // fecho a variavel compartilhada
      Serial.println("(TQ)Valvula SF: Fechada\n");
      shared_valvula_sai_fria = false;
    }
    xSemaphoreGive(tanque_frio); 
    
    xSemaphoreTake(temperatura, portMAX_DELAY );
    if(lim_min <= 10 | shared_temperatura < 70) { // acessa a temperatura
      Serial.println("(TQ)Valvula SQ: Fechada\n");
      valvula_sai_quente = false;
    }

    if(lim_min > 10 && lim_max < 90 && shared_temperatura >= 70) { // acessa a temperatura
      Serial.println("(TQ)Valvula SQ: Aberta\n");
      valvula_sai_quente = true;
    }
    xSemaphoreGive(aquecimento);
    vTaskDelay(17);
  }
}

static void task_aquecimento( void *pvParameters )
{
  bool var_aquecimento;
  
  while (7)
  {  
    xSemaphoreTake(aquecimento, portMAX_DELAY );
    shared_temperatura = 67;
    Serial.println("(AQ)Temperatura: ");
    Serial.println(shared_temperatura);
    Serial.println("\n");
    
    if(shared_temperatura >= 70) {  
      Serial.println("(AQ)Aquecimento: Desligado");
      var_aquecimento = false;
    } else {
      Serial.println("(AQ)Aquecimento: Ligado");
      var_aquecimento = true;
    }
    xSemaphoreGive(temperatura);
    vTaskDelay(9);
  }
}
