// Standard includes
#include "pebble.h"

	
// App-specific data
Window *window; // All apps must have at least one window
TextLayer *time_layer; 
TextLayer *hour_layer;
TextLayer *min_layer;
TextLayer *sec_layer; 
TextLayer *date_layer;
TextLayer *battery_layer;
TextLayer *today_layer;
static Layer *layer;
TextLayer *show_layer;
TextLayer *show2_layer;
static BitmapLayer *connect_layer;
static int waitForInit;
static int switcher;
static char shows1[256];
static char shows2[256];
static char hour_text[] = "00";
static char min_text[] = "00";
static char sec_text[] = "00";
static PropertyAnimation *property_animation;
Tuple *text_tuple;
Tuple *text2_tuple;
	
static GBitmap *connect;

static PropertyAnimation *out_animation;
static PropertyAnimation *in_animation;
static PropertyAnimation *out_animation2;
static PropertyAnimation *in_animation2;

static int waitForAnimation;

#define ANIMATION_DURATION_IN_MS 1000

static void init_animations() {
  Layer *layer = text_layer_get_layer(show_layer);
  Layer *layer2 = text_layer_get_layer(show2_layer);
	
  GRect START_POS = GRect(0, 100, 144, 68);	
  GRect TO_LEFT = GRect(-144, 100, 144, 68);
  GRect FROM_RIGHT = GRect(144, 100, 144, 68);	
	
  out_animation = property_animation_create_layer_frame(layer, &START_POS, &TO_LEFT);
  animation_set_duration((Animation*) out_animation, ANIMATION_DURATION_IN_MS);
  animation_set_curve((Animation*) out_animation, AnimationCurveEaseOut);

  in_animation = property_animation_create_layer_frame(layer2, &FROM_RIGHT, &START_POS);
  animation_set_duration((Animation*) in_animation, ANIMATION_DURATION_IN_MS);
  animation_set_curve((Animation*) in_animation, AnimationCurveEaseOut);
	
  out_animation2 = property_animation_create_layer_frame(layer2, &START_POS, &TO_LEFT);
  animation_set_duration((Animation*) out_animation2, ANIMATION_DURATION_IN_MS);
  animation_set_curve((Animation*) out_animation2, AnimationCurveEaseOut);

  in_animation2 = property_animation_create_layer_frame(layer, &FROM_RIGHT, &START_POS);
  animation_set_duration((Animation*) in_animation2, ANIMATION_DURATION_IN_MS);
  animation_set_curve((Animation*) in_animation2, AnimationCurveEaseOut);	
}

static void schedule_animation(int switcher_value) {
  int32_t animation_delay_required_in_ms = 100;

	if(waitForAnimation == 1){
		return;
	}
  if (switcher_value == 29 ) {
	//APP_LOG(APP_LOG_LEVEL_DEBUG,"Out animation layer 1");
	animation_set_duration((Animation*) out_animation, 700);   
    animation_set_delay((Animation*) out_animation, animation_delay_required_in_ms);
    animation_schedule((Animation*) out_animation);
  } else if (switcher_value == 30 ){
	//APP_LOG(APP_LOG_LEVEL_DEBUG,"In animation layer 2");
	animation_set_duration((Animation*) in_animation, 500);   
    animation_set_delay((Animation*) in_animation, animation_delay_required_in_ms);
    animation_schedule((Animation*) in_animation);
  }
  if (switcher_value == 59 ) {
	//APP_LOG(APP_LOG_LEVEL_DEBUG,"Out animation layer 2");
    animation_set_duration((Animation*) out_animation2, 700);   
	animation_set_delay((Animation*) out_animation2, animation_delay_required_in_ms);
    animation_schedule((Animation*) out_animation2);
  } else if (switcher_value == 0 ){
	//APP_LOG(APP_LOG_LEVEL_DEBUG,"In animation layer 1");
    animation_set_duration((Animation*) in_animation2, 500);   
	animation_set_delay((Animation*) in_animation2, animation_delay_required_in_ms);
    animation_schedule((Animation*) in_animation2);
  }	
}

const VibePattern BLUETOOTH_DISCONNECT_VIBE = {
  .durations = (uint32_t []) {100, 85, 100, 85, 100},
  .num_segments = 5
};

const VibePattern BLUETOOTH_CONNECT_VIBE = {
  .durations = (uint32_t []) {100, 85, 100},
  .num_segments = 3
};


void refreshShows(){
	//refresh tv shows
	DictionaryIterator *iter;
  	app_message_outbox_begin(&iter);
	
	if (clock_is_24h_style()) {
    	Tuplet value = TupletInteger(1, 1);
		dict_write_tuplet(iter, &value);
    } else {
    	Tuplet value = TupletInteger(1, 0);
		dict_write_tuplet(iter, &value);
    }
 	app_message_outbox_send();	
}

void in_received_handler(DictionaryIterator *received, void *context) {
    // Check for fields you expect to receive
	text_tuple = dict_find(received, 0);
    text2_tuple = dict_find(received, 1);
	Tuple *call_tuple = dict_find(received, 2);
	
	
	
	if (text_tuple) {
      snprintf(shows1, sizeof(shows1), "%s", text_tuple->value->cstring);
	  text_layer_set_text(show_layer, shows1);
	}
	if (text2_tuple) {
      snprintf(shows2, sizeof(shows2), "%s", text2_tuple->value->cstring);
	  text_layer_set_text(show2_layer, shows2);
	}
	if(call_tuple){
		snprintf(shows1, sizeof(shows1), "Refreshing");
	 	text_layer_set_text(show_layer, shows1);
		refreshShows();
	}
	waitForAnimation = 0; 
	switcher = 1;
 }

void in_dropped_handler(AppMessageResult reason, void *context) {
   //vibes_double_pulse();
 }

void out_sent_handler(DictionaryIterator *sent, void *context) {
   // outgoing message was delivered
 }


 void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
   // outgoing message failed
 }



static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[10];

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "charging");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d", charge_state.charge_percent);
  }
  text_layer_set_text(battery_layer, battery_text);
}

// Called once per second
static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
  char *hour_format;
  char *date_format;
 	
  if (clock_is_24h_style()) {
    	hour_format = "%R";
		date_format = "%d-%m-%Y";
    } else {
    	hour_format = "%I";
		date_format = "%m-%d-%Y";
    }
	//sec_format = "%S";
	
	switcher =  tick_time->tm_sec;
	if(units_changed & MINUTE_UNIT){
		strftime(min_text, sizeof(min_text), "%M", tick_time);
  		text_layer_set_text(min_layer, min_text);
    }
	if(units_changed & SECOND_UNIT){
		strftime(sec_text, sizeof(sec_text), "%S", tick_time);
  		text_layer_set_text(sec_layer, sec_text);
	}
	if (text2_tuple) {
	  if(switcher == 29){
		schedule_animation(switcher);
	  }
	  if(switcher == 30){
		schedule_animation(switcher);
	  }
	  if(switcher == 59){
		schedule_animation(switcher);
	  }
	  if(switcher == 0){
		schedule_animation(switcher);
	  }
	}
	
	if(units_changed & HOUR_UNIT){
		strftime(hour_text, sizeof(hour_text), hour_format, tick_time);
  		text_layer_set_text(hour_layer, hour_text);
		refreshShows();
	}
	
	handle_battery(battery_state_service_peek());
	if (units_changed & MONTH_UNIT) {
		static char date_text[20];
		strftime(date_text, sizeof(date_text), date_format, tick_time); // Month DD, YYYY
		text_layer_set_text(date_layer, date_text);
	}else if(units_changed & DAY_UNIT){
		static char date_text[20];
		strftime(date_text, sizeof(date_text), date_format, tick_time); // Month DD, YYYY
		text_layer_set_text(date_layer, date_text);
	}
	//switcher++;
}


void update_date_text(){
	time_t now = time(NULL);
    struct tm *current_time = localtime(&now);
	static char date_text[20];
	char *date_format;
	
    if (clock_is_24h_style()) {
    	date_format = "%d-%m-%Y";
    } else {
    	date_format = "%m-%d-%Y";
    }
  	strftime(date_text, sizeof(date_text), date_format, current_time); // Month DD, YYYY
  	text_layer_set_text(date_layer, date_text);
}


static void handle_bluetooth(bool connected) {
   if (connected) {
    vibes_enqueue_custom_pattern(BLUETOOTH_CONNECT_VIBE);
	connect = gbitmap_create_with_resource(RESOURCE_ID_CONNECTED);	
	if(connect){
	   free(connect);
	}   
	bitmap_layer_set_bitmap(connect_layer, connect);   
  } else{
    vibes_enqueue_custom_pattern(BLUETOOTH_DISCONNECT_VIBE);
	connect = NULL;	
	if(connect){
	   free(connect);
	}   
	bitmap_layer_set_bitmap(connect_layer, connect);   
  }
}


enum {
            AKEY_NUMBER,
            AKEY_TEXT,
        };




// Handle the start-up of the app
static void do_init(void) {
  // Create our app's base window
  window = window_create();
  window_stack_push(window, true);
  window_set_background_color(window, GColorBlack);
  switcher = 0;
	
  Layer *root_layer = window_get_root_layer(window);
  GRect frame = layer_get_frame(root_layer);
  GFont custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TTF_UBUNTU_30));
  GFont custom_font2 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TTF_UBUNTU_14));
  
  connect = gbitmap_create_with_resource(RESOURCE_ID_CONNECTED);	
  connect_layer = bitmap_layer_create(GRect(8, 2, /* width */ 16, 16 /* height */));	
  bitmap_layer_set_bitmap(connect_layer, connect);
  bitmap_layer_set_alignment(connect_layer, GAlignLeft);
  layer_add_child(root_layer, bitmap_layer_get_layer(connect_layer));	
	
  hour_layer = text_layer_create(GRect(9, 16, 64, 49));
  text_layer_set_text_color(hour_layer, GColorWhite);
  text_layer_set_background_color(hour_layer, GColorClear);
  text_layer_set_font(hour_layer, custom_font);
  text_layer_set_text_alignment(hour_layer, GTextAlignmentCenter);
	
  time_layer = text_layer_create(GRect(32, 15, 64, 49));
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_font(time_layer, custom_font);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_text(time_layer, ":");

  min_layer = text_layer_create(GRect(55, 16, 64, 49));
  text_layer_set_text_color(min_layer, GColorWhite);
  text_layer_set_background_color(min_layer, GColorClear);
  text_layer_set_font(min_layer, custom_font);
  text_layer_set_text_alignment(min_layer, GTextAlignmentCenter);
	
  sec_layer = text_layer_create(GRect(105, 32, frame.size.w, 34));
  text_layer_set_text_color(sec_layer, GColorWhite);
  text_layer_set_background_color(sec_layer, GColorClear);
  text_layer_set_font(sec_layer, custom_font2);
  text_layer_set_text_alignment(sec_layer, GTextAlignmentLeft);
	
  battery_layer = text_layer_create(GRect(-8, 1, /* width */ frame.size.w, 34 /* height */));
  text_layer_set_text_color(battery_layer, GColorWhite);
  text_layer_set_background_color(battery_layer, GColorClear);
  text_layer_set_font(battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(battery_layer, GTextAlignmentRight);
  text_layer_set_text(battery_layer, "100");
	
  date_layer = text_layer_create(GRect(0, 46, /* width */ frame.size.w, 34 /* height */));
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  text_layer_set_text(date_layer, "1-1-2013");
	
	
  today_layer = text_layer_create(GRect(3, 80, /* width */ frame.size.w, 34 /* height */));
  text_layer_set_text_color(today_layer, GColorWhite);
  text_layer_set_background_color(today_layer, GColorClear);
  text_layer_set_font(today_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(today_layer, GTextAlignmentCenter);
  text_layer_set_text(today_layer, "Upcoming shows:");		
	
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  int secs =  current_time->tm_sec;
  int show1pos = 0;
  int show2pos = 144;	
	if(secs > 29){
	  show1pos = 144;
	  show2pos = 0;	
	}		
	
  show_layer = text_layer_create(GRect(show1pos, 100, /* width */ frame.size.w, 68 /* height */));
  text_layer_set_text_color(show_layer, GColorWhite);
  text_layer_set_background_color(show_layer, GColorClear);
  text_layer_set_font(show_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(show_layer, GTextAlignmentLeft);
  text_layer_set_text(show_layer, "Waiting for refresh...");	

	
  show2_layer = text_layer_create(GRect(show2pos, 100, /* width */ frame.size.w, 68 /* height */));
  text_layer_set_text_color(show2_layer, GColorWhite);
  text_layer_set_background_color(show2_layer, GColorClear);
  text_layer_set_font(show2_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(show2_layer, GTextAlignmentLeft);
  text_layer_set_text(show2_layer, "");		
  
  waitForAnimation = 1; 	
  // Ensures time is displayed immediately (will break if NULL tick event accessed).
  // (This is why it's a good idea to have a separate routine to do the update itself.)
  
	  
  char *hour_format;
  if (clock_is_24h_style()) {
    	hour_format = "%R";
   } else {
    	hour_format = "%I";
    }	
  strftime(hour_text, sizeof(hour_text), hour_format, current_time);
  text_layer_set_text(hour_layer, hour_text);

  strftime(min_text, sizeof(min_text), "%M", current_time);
  text_layer_set_text(min_layer, min_text);	
	
  strftime(sec_text, sizeof(sec_text), "%S", current_time);
  text_layer_set_text(sec_layer, sec_text);
	
  //handle_second_tick(current_time, SECOND_UNIT);
  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(&handle_bluetooth);
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);	
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);	
	
	
  const uint32_t inbound_size = 255;
  const uint32_t outbound_size = 64;
  app_message_open(inbound_size, outbound_size);	
	
  layer_add_child(root_layer, text_layer_get_layer(time_layer));
  layer_add_child(root_layer, text_layer_get_layer(hour_layer));	
  layer_add_child(root_layer, text_layer_get_layer(min_layer));	
  layer_add_child(root_layer, text_layer_get_layer(sec_layer));	
  layer_add_child(root_layer, text_layer_get_layer(battery_layer));
  layer_add_child(root_layer, text_layer_get_layer(date_layer));

  layer_add_child(root_layer, text_layer_get_layer(today_layer));	
  layer_add_child(root_layer, text_layer_get_layer(show_layer));	
  layer_add_child(root_layer, text_layer_get_layer(show2_layer));		
  
  init_animations();
	
  update_date_text();
}



static void do_deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  
  accel_data_service_unsubscribe();
  text_layer_destroy(time_layer);
  text_layer_destroy(hour_layer);
  text_layer_destroy(min_layer);
  text_layer_destroy(sec_layer);	
  text_layer_destroy(battery_layer);
  text_layer_destroy(date_layer);
  
  text_layer_destroy(today_layer);
  text_layer_destroy(show_layer);
  text_layer_destroy(show2_layer);
	
  bitmap_layer_destroy(connect_layer);
  gbitmap_destroy(connect);	
  window_destroy(window);
}

// The main event/run loop for our app
int main(void) {
  do_init();
  app_event_loop();
  do_deinit();
}
