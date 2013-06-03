#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"




/**************************************************************************************************
* GLOBAL DEFINITIONS AND VARIABLES
**************************************************************************************************/
#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168
#define SCREEN_HEADER_HEIGHT 16

#define YOCTO_MANUFACTURER_LEN      20
#define YOCTO_SERIAL_LEN            20
#define YOCTO_BASE_SERIAL_LEN        8
#define YOCTO_PRODUCTNAME_LEN       28
#define YOCTO_FIRMWARE_LEN          22
#define YOCTO_LOGICAL_LEN           20
#define YOCTO_FUNCTION_LEN          20





enum {
  CMD_KEY = 0x0 // TUPLE_INTEGER
};

enum {
  PREVIOUS_RELAY    = 0x00,
  TOGGLE_RELAY      = 0x01,
  NEXT_RELAY        = 0x02,
  STARTED           = 0x03
};

enum {
  RELAY_STATE   = 0,
  RELAY_NAME    = 1,
  MODULE_NAME   = 2,
  PRODUCT_NAME  = 3
};



#define TEXT_LAYER_HEIGHT  (SCREEN_HEIGHT/4)


#define MY_UUID { 0x7D, 0x93, 0xA5, 0xD5, 0xEA, 0x73, 0x41, 0x92, 0xB3, 0x89, 0x01, 0xA9, 0x42, 0x42, 0xCA, 0x82 }
PBL_APP_INFO(MY_UUID,
             "Yocto-Relay", "Yoctopuce",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);

Window root_window;
static struct RelayInfo{
    Layer main_layer;
    char module_name[YOCTO_LOGICAL_LEN];
    TextLayer module_name_layer;
    char relay_name[YOCTO_LOGICAL_LEN];
    TextLayer relay_name_layer;
    char product[YOCTO_PRODUCTNAME_LEN];
    TextLayer product_layer;
    char state[YOCTO_PRODUCTNAME_LEN];
    TextLayer state_layer;
} relay_infos[2];
static int current_relay_info=0;
/**************************************************************************************************
* COMMUNICATION RELATED STUFF
**************************************************************************************************/






static int send_button(uint8_t cmd) {
  Tuplet value = TupletInteger(CMD_KEY, cmd);
  
  DictionaryIterator *iter;
  app_message_out_get(&iter);
  
  if (iter == NULL){
    //vibes_long_pulse();
    return -1;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);
  
  app_message_out_send();
  app_message_out_release();
  return 0;
}



static void app_send_done(DictionaryIterator* done, void* context) 
{
}

static void app_send_failed(DictionaryIterator* failed, AppMessageResult reason, void* context) {
    vibes_double_pulse();
}

static void app_received_done(DictionaryIterator* received, void* context) 
{
    int next_info=current_relay_info;//(current_relay_info+1)&1;
    
    Tuple* tuple = dict_read_first(received);
    while (tuple) {
        switch (tuple->key) {
        case RELAY_STATE:
            strcpy(relay_infos[next_info].state,tuple->value->cstring);
            layer_mark_dirty(&relay_infos[next_info].state_layer.layer);
            break;
        case RELAY_NAME:
            strcpy(relay_infos[next_info].relay_name,tuple->value->cstring);
            layer_mark_dirty(&relay_infos[next_info].relay_name_layer.layer);
            break;
        case MODULE_NAME:
            strcpy(relay_infos[next_info].module_name,tuple->value->cstring);
            layer_mark_dirty(&relay_infos[next_info].module_name_layer.layer);
            break;
        case PRODUCT_NAME:
            strcpy(relay_infos[next_info].product,tuple->value->cstring);
            strcat(relay_infos[next_info].product,":");
            layer_mark_dirty(&relay_infos[next_info].product_layer.layer);
            break;
        default:
            break;
        }
        tuple = dict_read_next(received);
    }
}

static void app_received_failed(void* context, AppMessageResult reason) {
  vibes_double_pulse();
}







/**************************************************************************************************
* BUTTONS HANDLERS
**************************************************************************************************/

void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) 
{
  send_button(PREVIOUS_RELAY);
}


void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) 
{
    send_button(TOGGLE_RELAY);
}

void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) 
{
  send_button(NEXT_RELAY);
}



void click_config_provider(ClickConfig **config, Window *window) {
  // See ui/click.h for more information and default values.

  // single click / repeat-on-hold config:
  config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
  config[BUTTON_ID_UP]->click.repeat_interval_ms = 1000; // "hold-to-repeat" gets overridden if there's a long click handler configured!

  // single click / repeat-on-hold config:
  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;
  config[BUTTON_ID_SELECT]->click.repeat_interval_ms = 1000; // "hold-to-repeat" gets overriden if there's a long click handler configured!

  // single click / repeat-on-hold config:
  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
  config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 1000; // "hold-to-repeat" gets overridden if there's a long click handler configured!
 
  (void)window;
}


/**************************************************************************************************
* INTITIALISATIONS STUFF
**************************************************************************************************/
#define H_PAD 2
#define V_PAD 2
#define COMPACT_HEIGHT 22
#define DOUBLE_HEIGHT  48




void initRelayInfo(int index)
{
  layer_init(&relay_infos[index].main_layer, GRect(0, 0, SCREEN_WIDTH , SCREEN_HEIGHT));


  // product Text Layer
  text_layer_init(&relay_infos[index].product_layer, GRect(H_PAD, 0, SCREEN_WIDTH-2*H_PAD , 22 ));
  text_layer_set_font(&relay_infos[index].product_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  strcpy(relay_infos[index].product,"Yocto-LatchedRelay:");
  text_layer_set_text(&relay_infos[index].product_layer,relay_infos[index].product);
  layer_add_child(&relay_infos[index].main_layer, &relay_infos[index].product_layer.layer);

  //module Text layer
  text_layer_init(&relay_infos[index].module_name_layer, GRect(H_PAD, 20, SCREEN_WIDTH-2*H_PAD , 28 ));
  text_layer_set_text_alignment(&relay_infos[index].module_name_layer,GTextAlignmentRight);
#if 0
  text_layer_set_text_color(&relay_infos[index].module_name_layer, GColorWhite);
  text_layer_set_background_color(&relay_infos[index].module_name_layer, GColorBlack);
#endif
  text_layer_set_font(&relay_infos[index].module_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  strcpy(relay_infos[index].module_name,"YMXCOUPL-00000");
  text_layer_set_text(&relay_infos[index].module_name_layer,relay_infos[index].module_name);
  layer_add_child(&relay_infos[index].main_layer, &relay_infos[index].module_name_layer.layer);


  // relay Text Layer
  text_layer_init(&relay_infos[index].relay_name_layer, GRect(0, 70 ,SCREEN_WIDTH, 50 ));
  text_layer_set_text_alignment(&relay_infos[index].relay_name_layer,GTextAlignmentCenter);
#if 1
  text_layer_set_text_color(&relay_infos[index].relay_name_layer, GColorWhite);
  text_layer_set_background_color(&relay_infos[index].relay_name_layer, GColorBlack);
#endif
  text_layer_set_font(&relay_infos[index].relay_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  strcpy(relay_infos[index].relay_name,"Loading.");
  text_layer_set_text(&relay_infos[index].relay_name_layer,relay_infos[index].relay_name);
  layer_add_child(&relay_infos[index].main_layer, &relay_infos[index].relay_name_layer.layer);


  // staet Text Layer
  text_layer_init(&relay_infos[index].state_layer, GRect(0, 100 ,SCREEN_WIDTH, 60 ));
  text_layer_set_text_alignment(&relay_infos[index].state_layer,GTextAlignmentCenter);
#if 1
  text_layer_set_text_color(&relay_infos[index].state_layer, GColorWhite);
  text_layer_set_background_color(&relay_infos[index].state_layer, GColorBlack);
#endif
  text_layer_set_font(&relay_infos[index].state_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  strcpy(relay_infos[index].state,"..");
  text_layer_set_text(&relay_infos[index].state_layer,relay_infos[index].state);
  layer_add_child(&relay_infos[index].main_layer, &relay_infos[index].state_layer.layer);
    
    
}

static int first_tick=1;
void handle_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)t;
  (void)ctx;
  if(first_tick && send_button(STARTED)==0){
    strcpy(relay_infos[0].relay_name,"Loading...");
    layer_mark_dirty(&relay_infos[0].relay_name_layer.layer);
    first_tick=0;
  }
}

void AppInitHandler(AppContextRef ctx) 
{
    (void)ctx;
    window_init(&root_window, "yRootWindow");
    window_set_background_color(&root_window, GColorBlack);
    initRelayInfo(0);
    initRelayInfo(1);
    layer_add_child(&root_window.layer, &relay_infos[current_relay_info].main_layer);
    // Attach our desired button functionality
    window_set_click_config_provider(&root_window, (ClickConfigProvider) click_config_provider);
    window_stack_push(&root_window, true /* Animated */);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &AppInitHandler,
    .tick_info = {
      .tick_handler = &handle_tick,    // called repeatedly, each second
      .tick_units = SECOND_UNIT        // specifies interval of `tick_handler`
    },
    .messaging_info = {
      .buffer_sizes = {
        .inbound = 256,
        .outbound = 32,
      },
      .default_callbacks.callbacks = {
        .out_sent = &app_send_done,
        .out_failed = &app_send_failed,
        .in_received = &app_received_done,
        .in_dropped = &app_received_failed,
      },
    }

  };
  app_event_loop(params, &handlers);
}
