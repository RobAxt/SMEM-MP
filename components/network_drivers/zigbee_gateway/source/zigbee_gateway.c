#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_zigbee_core.h"
#include "esp_zigbee_type.h"
#include "esp_zigbee_endpoint.h"
#include "nwk/esp_zigbee_nwk.h"
#include "zboss_api.h"
#include "esp_log.h"

#include "zigbee_gateway.h"


/** @brief Etiqueta para los logs del coordinador */

static const char *TAG = "zb_coord";

/**
 * @name Configuración del Coordinador Zigbee
 * @{
 */

/** @brief ID del cluster personalizado para comunicación de datos */
#define CLUSTER_ID          0xFC00

/** @brief Canal Zigbee donde se forma la red (canales válidos: 11-26) */
#define ZIGBEE_CHANNEL      11

/** @brief Intervalo entre lecturas de datos en milisegundos (5 segundos) */
static uint32_t POLL_INTERVAL_MS  = 5000;

/** @} */

/**
 * @name Variables Globales de Estado
 * @{
 */

/**
 * @brief Dirección del dispositivo end device conectado
 * @note 0x0000 significa ningún dispositivo conectado
 */
static uint16_t connected_ed_addr = 0x0000;

/**
 * @brief Flag que indica si hay una operación de lectura en progreso
 * @note Evita enviar múltiples peticiones simultáneas
 */
static bool read_in_progress = false;

/**
 * @brief Flag que indica si el polling periódico está activo
 * @note Se activa cuando un dispositivo se conecta
 */
static bool polling_active = false;

/**
 * @brief Valor del estado leído del dispositivo end device
 * @note Se actualiza con cada lectura exitosa
 */
static uint8_t state_value = 0;

/** @} */

/** @brief Declaración forward de la función de callback de polling */
static void read_state_callback(uint8_t param);

/**
 * @brief Callback que maneja las respuestas a las peticiones de lectura de atributos
 *
 * Esta función se ejecuta cuando un dispositivo end device responde a una petición
 * de lectura de atributos ZCL (Zigbee Cluster Library). Procesa la respuesta,
 * extrae los datos del sensor y programa la siguiente lectura.
 *
 * @param message Puntero al mensaje de respuesta recibido
 * @return ESP_OK si el procesamiento fue exitoso
 *
 * @note Esta función es crítica para el funcionamiento del polling continuo
 */
static esp_err_t zb_read_attr_resp_handler(const esp_zb_zcl_cmd_read_attr_resp_message_t *message)
{
    ESP_LOGD(TAG, "📥 Handler de respuesta llamado - status: %x, addr: 0x%04x", 
             message->info.status, message->info.src_address.u.short_addr);
    
    // Liberar el flag de lectura en progreso
    read_in_progress = false;
    
    // Verificar si la respuesta fue exitosa
    if (message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS) {
        // Procesar variables de atributos en la respuesta
        esp_zb_zcl_read_attr_resp_variable_t *variable = message->variables;
        
        while (variable) {
            // Buscar el atributo de interés (ID 0x0000, tipo uint8)
            if (variable->attribute.id == 0x0000 && 
                variable->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U8) {
                
                // Extraer el valor del sensor
                state_value = variable->attribute.data.value ? 
                                     *(uint8_t*)variable->attribute.data.value : 0;
                
                // Log del valor recibido
                ESP_LOGD(TAG, "📊 Estado recibido de 0x%04x: 0x%02X (%u)",
                         message->info.src_address.u.short_addr, state_value, state_value);
            }
            // Siguiente atributo
            variable = variable->next;
        }
        
        // Programar siguiente lectura si polling sigue activo
        if (polling_active) {
            ESP_LOGD(TAG, "⏰ Programando siguiente lectura en %ld ms", (long)POLL_INTERVAL_MS);
            esp_zb_scheduler_alarm((esp_zb_callback_t)read_state_callback, 0, POLL_INTERVAL_MS);
        }
    } else {
        // Log de error de lectura
        ESP_LOGW(TAG, "⚠️  Error al leer de 0x%04x: status=%u",
                 message->info.src_address.u.short_addr, (unsigned)message->info.status);
 
    }
    
    return ESP_OK;
}

/**
 * @brief Callback principal para manejar todos los comandos ZCL (Zigbee Cluster Library)
 *
 * Esta función actúa como un despachador que recibe todos los mensajes ZCL
 * y los dirige al handler específico según el tipo de comando.
 *
 * @param callback_id Identificador del tipo de callback
 * @param message Puntero al mensaje recibido
 * @return ESP_OK si el procesamiento fue exitoso
 */
static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    // Despachar según el tipo de comando recibido
    if (callback_id == ESP_ZB_CORE_CMD_READ_ATTR_RESP_CB_ID) {
        return zb_read_attr_resp_handler((esp_zb_zcl_cmd_read_attr_resp_message_t *)message);
    }
    
    return ESP_OK;
}

/**
 * @brief Handler principal de señales del stack Zigbee
 *
 * Esta función es el corazón del coordinador Zigbee. Maneja todos los eventos
 * importantes del stack Zigbee como:
 * - Inicialización del stack
 * - Formación de la red
 * - Conexión de dispositivos
 * - Desconexión de dispositivos
 *
 * Cada señal representa un cambio de estado en la red Zigbee y requiere
 * una respuesta específica del coordinador.
 *
 * @param signal Puntero a la estructura de señal recibida
 *
 * @note Esta función se ejecuta automáticamente cuando ocurren eventos Zigbee
 */
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal)
{
    // Extraer información de la señal
    esp_err_t err_status = signal->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *signal->p_app_signal;
    
    // Procesar según el tipo de señal recibida
    switch (sig_type) {
    // Stack Zigbee inicializado y listo
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Stack Zigbee inicializado");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
        
    // Primer inicio o reinicio del dispositivo
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK) {
            ESP_LOGI(TAG, "Formando red Zigbee...");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_FORMATION);
        } else {
            ESP_LOGE(TAG, "Error al iniciar: %s", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)esp_zb_bdb_start_top_level_commissioning,
                                   ESP_ZB_BDB_MODE_INITIALIZATION, 1000);
        }
        break;
        
    // Red Zigbee formada exitosamente
    case ESP_ZB_BDB_SIGNAL_FORMATION:
        if (err_status == ESP_OK) {
            ESP_LOGI(TAG, "Red formada - PAN ID: 0x%04hx, Canal: %u", 
                     esp_zb_get_pan_id(), (unsigned)esp_zb_get_current_channel());
            // Imprimir la dirección IEEE real del coordinador
            uint8_t ieee_addr[8] = {0};
            esp_zb_get_long_address(ieee_addr);
            char ieee_str[3*8] = {0};
            for (int i = 0; i < 8; ++i) {
                sprintf(ieee_str + i*3, "%02X%s", ieee_addr[7-i], (i < 7) ? ":" : "");
            }
            ESP_LOGI(TAG, "IEEE Address: %s", ieee_str);
            ESP_LOGI(TAG, "Abriendo red para dispositivos...");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        } else {
            ESP_LOGE(TAG, "Error al formar red: %s", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)esp_zb_bdb_start_top_level_commissioning,
                                   ESP_ZB_BDB_MODE_NETWORK_FORMATION, 1000);
        }
        break;
        
    // Red abierta y lista para aceptar dispositivos
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK) {
            ESP_LOGI(TAG, "Red abierta para unión de dispositivos");
        }
        break;
        
    // Un dispositivo se ha unido exitosamente a la red
    case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        {
            esp_zb_zdo_signal_device_annce_params_t *dev_annce_params = 
                (esp_zb_zdo_signal_device_annce_params_t *)esp_zb_app_signal_get_params(signal->p_app_signal);
            ESP_LOGI(TAG, "✅ Dispositivo unido - addr: 0x%04hx", dev_annce_params->device_short_addr);
            
            // Actualizar dirección del dispositivo conectado
            connected_ed_addr = dev_annce_params->device_short_addr;
            read_in_progress = false;  // Resetear flag por si había lectura pendiente
            
            // Reiniciar polling en cada reconexión para asegurar funcionamiento
            polling_active = true;
            ESP_LOGI(TAG, "Iniciando/reiniciando polling cada %ld segundos...", (long)POLL_INTERVAL_MS / 1000);
            esp_zb_scheduler_alarm((esp_zb_callback_t)read_state_callback, 0, POLL_INTERVAL_MS);
        }
        break;
        
    // Un dispositivo se está asociando a la red (antes de unirse completamente)
    case ESP_ZB_NWK_SIGNAL_DEVICE_ASSOCIATED:
        ESP_LOGI(TAG, "📱 Dispositivo asociándose a la red");
        break;
        
    // Un dispositivo ha dejado la red
    case ESP_ZB_ZDO_SIGNAL_LEAVE:
        ESP_LOGW(TAG, "❌ Dispositivo dejó la red");
        // Limpiar la dirección para permitir reconexión
        connected_ed_addr = 0x0000;
        read_in_progress = false;
        polling_active = false;  // Permitir reiniciar polling cuando se reconecte
        ESP_LOGI(TAG, "Esperando nuevo dispositivo...");
        break;
    
       case ESP_ZB_NLME_STATUS_INDICATION:
            esp_zb_zdo_signal_nwk_status_indication_params_t *nlme =
                (esp_zb_zdo_signal_nwk_status_indication_params_t *)esp_zb_app_signal_get_params(signal->p_app_signal);
            ESP_LOGW(TAG, "� NLME_STATUS_INDICATION: nwk_status=0x%02X, nwk_addr=0x%04X, unknown_cmd=%u",
                     nlme->status, nlme->network_addr, nlme->unknown_command_id);

            // Código 0x09 = NWK_NO_ROUTE (no hay ruta al dispositivo)
            if (nlme->status == ESP_ZB_NWK_COMMAND_STATUS_PARENT_LINK_FAILURE) 
            {
                ESP_LOGW(TAG, "🚫 No hay ruta al dispositivo 0x%04X - posible congestión de red", nlme->network_addr);
            }
        break;
    
    case ESP_ZB_ZDO_DEVICE_UNAVAILABLE:
        esp_zb_zdo_device_unavailable_params_t *unavail_params =
            (esp_zb_zdo_device_unavailable_params_t *)esp_zb_app_signal_get_params(signal->p_app_signal);
        ESP_LOGW(TAG, "Dispositivo no disponible - addr: 0x%04hx", unavail_params->short_addr);

        break;
    default:
        ESP_LOGI(TAG, "Unhandled ZDO signal: %u, status: 0x%x", (unsigned)sig_type, (unsigned)err_status);
        break;
    }
}

/**
 * @brief Crea la lista de clusters que soporta el coordinador
 *
 * Los clusters definen qué funcionalidades tiene el dispositivo Zigbee.
 * En este caso, el coordinador tiene:
 * - Cluster básico (información del dispositivo)
 * - Cluster custom para comunicación de datos
 *
 * @return Puntero a la lista de clusters creada
 */
static esp_zb_cluster_list_t *coordinator_clusters_create(void)
{
    // Crear lista vacía de clusters
    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();
    
    // Agregar cluster básico (obligatorio para todos los dispositivos Zigbee)
    esp_zb_attribute_list_t *basic_cluster = esp_zb_basic_cluster_create(NULL);
    esp_zb_cluster_list_add_basic_cluster(cluster_list, basic_cluster, ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
    
    // Agregar cluster custom para comunicación de datos del sensor
    esp_zb_attribute_list_t *custom_cluster = esp_zb_zcl_attr_list_create(CLUSTER_ID);
    esp_zb_cluster_list_add_custom_cluster(cluster_list, custom_cluster, ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
    
    return cluster_list;
}

/**
 * @brief Crea el endpoint principal del coordinador
 *
 * Un endpoint es como una "interfaz" del dispositivo Zigbee. Define qué
 * clusters están disponibles en ese endpoint y qué perfil usa.
 *
 * @return Puntero a la lista de endpoints creada
 */
static esp_zb_ep_list_t *coordinator_ep_create(void)
{
    // Crear lista vacía de endpoints
    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    
    // Configurar endpoint 1 con perfil Home Automation
    esp_zb_endpoint_config_t endpoint_config = {
        .endpoint = 1,                                    // Número del endpoint
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,         // Perfil Home Automation
        .app_device_id = ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID, // Tipo: sensor simple
        .app_device_version = 0                            // Versión del dispositivo
    };
    
    // Agregar endpoint con sus clusters
    esp_zb_ep_list_add_ep(ep_list, coordinator_clusters_create(), endpoint_config);
    return ep_list;
}

/**
 * @brief Callback que se ejecuta periódicamente para hacer polling de datos
 *
 * Esta función se ejecuta cada 5 segundos (o intervalos variables) y envía
 * una petición de lectura de atributos al dispositivo end device conectado.
 * Es el corazón del mecanismo de polling continuo.
 *
 * @param param Parámetro no usado (requerido por el scheduler)
 */
static void read_state_callback(uint8_t param)
{
    ESP_LOGD(TAG, "🔄 Callback de polling ejecutado - addr: 0x%04x, read_in_progress: %u",
             connected_ed_addr, (unsigned)read_in_progress);
    
    // Verificar que hay un dispositivo conectado y polling activo
    if (connected_ed_addr != 0x0000 && polling_active) {
        uint16_t attr_id = 0x0000;
        esp_zb_zcl_read_attr_cmd_t read_req = {
            .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
            .clusterID = CLUSTER_ID,
            .zcl_basic_cmd = {
                .dst_addr_u.addr_short = connected_ed_addr,
                .dst_endpoint = 1,
                .src_endpoint = 1,
            },
            .attr_number = 1,
            .attr_field = &attr_id,
        };
        ESP_LOGD(TAG, "📤 Enviando petición de lectura a 0x%04x (cluster: 0x%04x, attr: 0x%04x, endpoint: %u->%u)", 
                 connected_ed_addr, CLUSTER_ID, attr_id, read_req.zcl_basic_cmd.src_endpoint, read_req.zcl_basic_cmd.dst_endpoint);
        esp_zb_zcl_read_attr_cmd_req(&read_req);
        ESP_LOGD(TAG, "✅ Petición enviada");
    } else {
        ESP_LOGW(TAG, "⚠️  No hay dispositivo conectado, saltando petición");
    }
}

/**
 * @brief Función principal del stack Zigbee
 * @details Esta función ejecuta el loop principal del stack Zigbee.
 */
static void zb_stack_main_loop_task(void *pvParameters)
{
    // Entrar al loop principal del stack Zigbee (esta función nunca retorna)
    esp_zb_stack_main_loop();
}

/**
 * @brief funcion de inicialización del gateway zigbee
 * @return esp_err_t código de error de la operación
  
*/

esp_err_t zigbee_gateway_start(void)
{
    esp_err_t err = ESP_OK;
    // Configurar el dispositivo como coordinador Zigbee
    esp_zb_cfg_t zb_cfg = {
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_COORDINATOR,  // Rol: Coordinador
        .nwk_cfg.zczr_cfg.max_children = 16,             // Máximo 16 dispositivos hijos
    };

    // Inicializar el stack Zigbee
    ESP_LOGD(TAG, "Inicializando stack Zigbee...");
    esp_zb_init(&zb_cfg);
    
    // Registrar los endpoints y clusters que soporta este dispositivo
    err = esp_zb_device_register(coordinator_ep_create());
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al registrar el dispositivo Zigbee: %d", err);
        return err;
    }

    // Configurar el canal Zigbee (solo canal 11)
    err = esp_zb_set_channel_mask(1 << ZIGBEE_CHANNEL);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al configurar el canal Zigbee: %d", err);
        return err;
    }

    // Habilitar soporte para dispositivos legacy (más compatibilidad)
    zb_bdb_set_legacy_device_support(1);
    
    // Deshabilitar política de install code (para desarrollo)
    zb_set_installcode_policy(false);
    
    ESP_LOGD(TAG, "Canal: %u, Max children: %u", (unsigned)ZIGBEE_CHANNEL, 16);

    // Registrar el handler para comandos ZCL (respuestas de lectura)
    esp_zb_core_action_handler_register(zb_action_handler);
  
    // Iniciar el stack Zigbee (false = no esperar por formación de red)
    ESP_ERROR_CHECK(esp_zb_start(false));    
    
    // Crear tarea Zigbee (stack) y tarea 
    xTaskCreate(zb_stack_main_loop_task, "zb_stack_main_loop_task", 4096, NULL, 5, NULL); 
    
    return ESP_OK;
}

esp_err_t zigbee_gateway_data_receive(uint8_t *data, size_t length)
{
    if(data == NULL || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *data = state_value;

    return ESP_OK;
}
