#include <pebble.h>
#include "main.h"
#include "weekday.h"
#include "effect_layer.h"

//Static and initial vars
static GFont FontHour, FontDate, FontWeather, FontSteps, FontIcon, FontIcon2;
char tempstring[44], condstring[44], citistring[44], windstring[44], sunsetstring[44];

static Window * s_window;

EffectLayer* effect_layer;
EffectLayer* effect_layer_2; //rotation effect layer
EffectLayer* effect_layer_3; //blur effect layer

GColor color_loser;
GColor color_winner;

static Layer * s_canvas_to_be_rotated;
static Layer * s_canvas;
static Layer * s_canvas_complications;
static Layer * s_canvas_sunset_icon;
static Layer * s_canvas_bt_icon;
static TextLayer *s_step_layer;
//static TextLayer *s_bt_icon_layer2;
//static TextLayer *s_sunset_icon_layer2;

static int s_hours, s_minutes, s_weekday, s_day, s_loop, s_countdown;
//////Init Configuration///
//Init Clay
ClaySettings settings;
// Initialize the default settings
static void prv_default_settings(){
  settings.Back1Color = GColorBlack;
  settings.Text1Color = GColorWhite;
  settings.Text2Color = GColorWhite;
  settings.Text3Color = GColorWhite;
  settings.HourColor = GColorCyan;
  settings.MinColor = GColorPastelYellow;
  settings.HourColorN = GColorDarkCandyAppleRed;
  settings.MinColorN = GColorBlack;
  settings.Back1ColorN = GColorWhite;
  settings.Text1ColorN = GColorBlack;
  settings.Text2ColorN = GColorBlack;
  settings.Text3ColorN = GColorBlack; 
  settings.WeatherUnit = 0;
  settings.UpSlider = 30;
  settings.NightTheme = true;
}
int HourSunrise=700;
int HourSunset=2200;
bool BTOn=true;
bool GPSOn=true;
bool IsNightNow=false;
//////End Configuration///
///////////////////////////

static GColor ColorSelect(GColor ColorDay, GColor ColorNight){
  if (settings.NightTheme && IsNightNow && GPSOn ){
    return ColorNight;
  } 
  else{
    return ColorDay;
  }
}
// Callback for js request
void request_watchjs(){
  //Starting loop at 0
  s_loop = 0;
  // Begin dictionary
  DictionaryIterator * iter;
  app_message_outbox_begin( & iter);
  // Add a key-value pair
  dict_write_uint8(iter, 0, 0);
  // Send the message!
  app_message_outbox_send();
}

///BT Connection
static void bluetooth_callback(bool connected){
  BTOn = connected;
}

static void bluetooth_vibe_icon (bool connected) {

  // Show icon if disconnected, hiding sunset icon
 // layer_set_hidden(text_layer_get_layer(s_bt_icon_layer2), connected);
 // layer_set_hidden(text_layer_get_layer(s_sunset_icon_layer2), !connected);
  
  layer_set_hidden(s_canvas_bt_icon, connected);
  layer_set_hidden(s_canvas_sunset_icon, !connected);

  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}

static void onreconnection(bool before, bool now){
  if (!before && now){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "BT reconnected, requesting weather at %d", s_minutes);
    request_watchjs();
  }
}


//Add in HEALTH to the watchface
static char s_current_steps_buffer[16];
static int s_step_count = 0, s_step_goal = 0, s_step_average = 0;

// Is step data available?
bool step_data_is_available() {
  return HealthServiceAccessibilityMaskAvailable &
    health_service_metric_accessible(HealthMetricStepCount,
      time_start_of_today(), time(NULL));
}

// Daily step goal
static void get_step_goal() {
  const time_t start = time_start_of_today();
  const time_t end = start + SECONDS_PER_DAY;
  s_step_goal = (int)health_service_sum_averaged(HealthMetricStepCount,
    start, end, HealthServiceTimeScopeDaily);
}

// Todays current step count
static void get_step_count() {
  //if(tick_time->tm_min%30==0) {
    s_step_count = (int)health_service_sum_today(HealthMetricStepCount);
}
//}
// Average daily step count for this time of day
static void get_step_average() {
  const time_t start = time_start_of_today();
  const time_t end = time(NULL);
  s_step_average = (int)health_service_sum_averaged(HealthMetricStepCount,
    start, end, HealthServiceTimeScopeDaily);
}

static void display_step_count() {
  int thousands = s_step_count / 1000;
  int hundreds = s_step_count % 1000;
  static char s_emoji[5];

  if(s_step_count >= s_step_average) {
    text_layer_set_text_color(s_step_layer, PBL_IF_BW_ELSE(ColorSelect(settings.Text1Color, settings.Text1ColorN), color_winner));
    snprintf(s_emoji, sizeof(s_emoji), "\U0001F600");
  } else {
    text_layer_set_text_color(s_step_layer, PBL_IF_BW_ELSE(ColorSelect(settings.Text1Color, settings.Text1ColorN), color_loser));
    snprintf(s_emoji, sizeof(s_emoji), "\U0001F61E");
  }

  if(thousands > 0) {
    snprintf(s_current_steps_buffer, sizeof(s_current_steps_buffer),
      "%d,%03d %s", thousands, hundreds, s_emoji);
  } else {
    snprintf(s_current_steps_buffer, sizeof(s_current_steps_buffer),
      "%d %s", hundreds, s_emoji);
  }
  text_layer_set_text(s_step_layer, s_current_steps_buffer);
}



static void health_handler(HealthEventType event, void *context) {
  if(event == HealthEventSignificantUpdate) {
    get_step_goal();
  }

  if(event != HealthEventSleepUpdate) {
    get_step_count();
    get_step_average();
    display_step_count();

  }
}


//Update main layer

static void layer_update_proc_pre_rotate(Layer * layer1, GContext * ctx1){
  // Create Rects
  GRect bounds1 = layer_get_bounds(layer1);
  
  GRect MediumBand =
    PBL_IF_ROUND_ELSE(
      GRect(0,0,180,180),
      GRect(0, 0, (bounds1.size.w), bounds1.size.h));
  
  GRect CondRect =  
    (PBL_IF_ROUND_ELSE(
        (GRect(bounds1.size.w/2 - 58,90-9,116,20)),
        (GRect(bounds1.size.w/2 - 38, 84-7, 76, 20 ))));
  
  
  GRect TempRect =  
    (PBL_IF_ROUND_ELSE(
      (GRect(1,90-9,32,20)),
      (GRect(0, 84-7, 32, 20))));
    
    
   GRect WindRect =  
    (PBL_IF_ROUND_ELSE(
      (GRect(180-33,90-8,32,20)),
      (GRect(144-32, 84-7, 32, 20))));
  
  onreconnection(BTOn, connection_service_peek_pebble_app_connection());
  bluetooth_callback(connection_service_peek_pebble_app_connection());
  
  char TempToDraw[6];
  char CondToDraw[20];
  char WindToDraw[20];

  if (!BTOn){
    snprintf(TempToDraw, sizeof(TempToDraw), " ");
    snprintf(WindToDraw, sizeof(WindToDraw), " ");
    snprintf(CondToDraw, sizeof(CondToDraw), " ");
  }
  else if (!GPSOn){
    snprintf(TempToDraw, sizeof(TempToDraw), " ");
    snprintf(WindToDraw, sizeof(WindToDraw), " ");
    snprintf(CondToDraw, sizeof(CondToDraw), " ");    
  }
  else {
    snprintf(TempToDraw, sizeof(TempToDraw), "%s",tempstring);
    snprintf(CondToDraw, sizeof(CondToDraw), "%s",condstring); 
    snprintf(WindToDraw, sizeof(WindToDraw), "%s",windstring); 
   }
  //Build display
  graphics_context_set_fill_color(ctx1, ColorSelect(settings.Back1Color, settings.Back1ColorN));
  graphics_fill_rect(ctx1, bounds1, 0, GCornerNone);
  graphics_context_set_fill_color(ctx1, ColorSelect(settings.Back1Color, settings.Back1ColorN));
  graphics_fill_rect(ctx1, MediumBand,0,GCornerNone);
  
  graphics_context_set_text_color(ctx1, ColorSelect(settings.Text1Color, settings.Text1ColorN));
  graphics_draw_text(ctx1, TempToDraw, FontWeather, TempRect, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx1, CondToDraw, FontWeather, CondRect, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx1, WindToDraw, FontWeather, WindRect, GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
   
}



static void layer_update_proc(Layer * layer, GContext * ctx){
  // Create Rects
  GRect bounds3 = layer_get_bounds(layer);

  GRect HourRect =  
    (PBL_IF_ROUND_ELSE(
      GRect(0, (bounds3.size.h/2) - 56, (bounds3.size.w/2)-10, 90),
      GRect(0-10, (bounds3.size.h/2) - 48, (bounds3.size.w/2), 90)));
  
  GRect MinRect = 
    (PBL_IF_ROUND_ELSE(
      GRect((bounds3.size.w/2)+10, (bounds3.size.h/2)-56, (bounds3.size.w/2)-1, 90),
      GRect((bounds3.size.w/2), (bounds3.size.h/2)-48, (bounds3.size.w/2), 90)));
  
  //int offsetdate = PBL_IF_RECT_ELSE(12, 10);
 

//Minutes and Hours 
  int mindraw;
  mindraw = s_minutes;
  char minnow[8];
  snprintf(minnow, sizeof(minnow), "%02d", mindraw);
  
  int hourdraw;
  char hournow[8];
  if (clock_is_24h_style()){
    hourdraw=s_hours;
    snprintf(hournow,sizeof(hournow),"%02d",hourdraw);
    }
  else {
    if (s_hours==0 || s_hours==12){
      hourdraw=12;
    }
    else hourdraw=s_hours%12;    
  snprintf(hournow, sizeof(hournow), "%d", hourdraw);
 // hourdraw = hourdraw1+(('0'==hourdraw1[0])?1:0));
  }
 
   //Date
  // Local language
   
  //Hour Band
  graphics_context_set_text_color(ctx, ColorSelect(settings.HourColor, settings.HourColorN));
  if(clock_is_24h_style()){
    graphics_draw_text(ctx,hournow,FontHour,HourRect,GTextOverflowModeFill,GTextAlignmentRight,NULL);
  }
  else{
    graphics_draw_text(ctx, hournow+(('0'==hournow[0])?1:0), FontHour, HourRect, GTextOverflowModeFill, PBL_IF_ROUND_ELSE(GTextAlignmentRight,GTextAlignmentRight), NULL); 
  } //graphics_draw_text(ctx, "47", FontHour, HourRect, GTextOverflowModeFill, PBL_IF_ROUND_ELSE(GTextAlignmentRight,GTextAlignmentRight), NULL); 
 
  //Min Band
  graphics_context_set_text_color(ctx, ColorSelect(settings.MinColor, settings.MinColorN));
  graphics_draw_text(ctx, minnow, FontHour, MinRect, GTextOverflowModeFill, PBL_IF_ROUND_ELSE(GTextAlignmentLeft,GTextAlignmentRight), NULL); 
   
   
}
static void  layer_update_proc_complications (Layer * layer4, GContext * ctx4){
  GRect bounds6 = layer_get_bounds(layer4);
  
   GRect DateRect = 
  //  (0, offsetdate, bounds3.size.w, bounds1.size.h/4);
    (PBL_IF_ROUND_ELSE(
      GRect((bounds6.size.w/2)+12, 122, 46, 20),
      GRect(96, 126, 46, 20)));
  
  GRect SunsetRect = 
    (PBL_IF_ROUND_ELSE(
      GRect(0+22,122,bounds6.size.w/2-10-22,20),
      GRect(22,126, bounds6.size.w/2,20)));
    
 GRect CitiRect =
    (PBL_IF_ROUND_ELSE(
      GRect(28, 142, bounds6.size.w/2-10-28,40),
      GRect(0,bounds6.size.h-20, bounds6.size.w/2-10, 22-2)));
  
 GRect BatteryRect =
   (PBL_IF_ROUND_ELSE(
//    GRect(55+27+18,140, 40, 22),
      GRect((bounds6.size.w/2)+12,142,bounds6.size.w,22),
      GRect((bounds6.size.w/2),bounds6.size.h - 20, bounds6.size.w/2, 20)));
  
  const char * sys_locale = i18n_get_system_locale();
  char datenow[10];
  fetchwday(s_weekday, sys_locale, datenow);
  char convertday[4];
  snprintf(convertday, sizeof(convertday), " %02d", s_day);
  // Concatenate date
  strcat(datenow, convertday);
  //Battery
  int battery_level = battery_state_service_peek().charge_percent;
  char battperc[20];
  snprintf(battperc, sizeof(battperc), "%d", battery_level);
  strcat(battperc, "%");
  // Draw AM PM 24H
  /*char ampm[5];
  if (clock_is_24h_style()){
    snprintf(ampm, sizeof(ampm), "24H");
  }
  else if (s_hours<12){
    snprintf(ampm, sizeof(ampm), "AM");    
  }
  else {
    snprintf(ampm, sizeof(ampm), "PM"); 
  }*/
  //Connection settings
  // Prepare to draw
  // Update connection toggle
  onreconnection(BTOn, connection_service_peek_pebble_app_connection());
  bluetooth_callback(connection_service_peek_pebble_app_connection());
  
  char LocToDraw[20];
  char SunsetToDraw[20];
  if (!BTOn){
    snprintf(LocToDraw, sizeof(LocToDraw), " ");
    snprintf(SunsetToDraw, sizeof(SunsetToDraw),  "%s", " ");
  }
  else if (!GPSOn){
 
    snprintf(LocToDraw, sizeof(LocToDraw), " ");
    snprintf(SunsetToDraw, sizeof(SunsetToDraw),  "%s", " ");    
  }
  else {
    snprintf(LocToDraw, sizeof(LocToDraw), "%s",citistring);
    snprintf(SunsetToDraw, sizeof(SunsetToDraw), "%s",sunsetstring); 
  }
    
  //dates band
  graphics_context_set_text_color(ctx4, ColorSelect(settings.Text2Color, settings.Text2ColorN));
  graphics_draw_text(ctx4, SunsetToDraw, FontDate, SunsetRect, GTextOverflowModeFill, PBL_IF_ROUND_ELSE(GTextAlignmentRight,GTextAlignmentLeft), NULL); 
  graphics_draw_text(ctx4,LocToDraw, FontWeather, CitiRect, GTextOverflowModeTrailingEllipsis, PBL_IF_ROUND_ELSE(GTextAlignmentRight,GTextAlignmentLeft), NULL); 
 
    
  graphics_context_set_text_color(ctx4, ColorSelect(settings.Text3Color, settings.Text3ColorN));
  graphics_draw_text(ctx4, datenow, FontDate, DateRect, GTextOverflowModeTrailingEllipsis, PBL_IF_ROUND_ELSE(GTextAlignmentLeft,GTextAlignmentRight), NULL);
  graphics_draw_text(ctx4, battperc, FontWeather, BatteryRect, GTextOverflowModeWordWrap, PBL_IF_ROUND_ELSE(GTextAlignmentLeft, GTextAlignmentRight), NULL);
}

static void layer_update_proc_sunset(Layer * layer2, GContext * ctx2){
   // Create Rects
//  GRect bounds = layer_get_bounds(layer2);
 
  GRect SunsetIconRect = 
    (PBL_IF_ROUND_ELSE(
      GRect(28,126,20,20),
      GRect(0,128+3,20,20)));
 
 onreconnection(BTOn, connection_service_peek_pebble_app_connection());
 bluetooth_callback(connection_service_peek_pebble_app_connection());
  
  char SunsetIconToDraw[20];
  if (!BTOn){
       snprintf(SunsetIconToDraw, sizeof(SunsetIconToDraw),  "%s"," ");
  }
  else if (!GPSOn){
    snprintf(SunsetIconToDraw, sizeof(SunsetIconToDraw),  "%s", " ");    
  }
  else if (strcmp(sunsetstring, " ") == 0){
    snprintf(SunsetIconToDraw, sizeof(SunsetIconToDraw),  "%s", " ");    
  }
  else {
    snprintf(SunsetIconToDraw, sizeof(SunsetIconToDraw), "%s","\U0000F052"); 
  }

 graphics_context_set_text_color(ctx2, ColorSelect(settings.Text3Color, settings.Text3ColorN));
 graphics_draw_text(ctx2, SunsetIconToDraw, FontIcon, SunsetIconRect, GTextOverflowModeFill,GTextAlignmentCenter, NULL);
 
}

static void layer_update_proc_bt(Layer * layer3, GContext * ctx3){
   // Create Rects
//  GRect bounds = layer_get_bounds(layer2);
 
  GRect BTIconRect = 
    (PBL_IF_ROUND_ELSE(
      GRect(28,126,20,20),
      GRect(0,128+3,20,20)));
  
 onreconnection(BTOn, connection_service_peek_pebble_app_connection());
 bluetooth_callback(connection_service_peek_pebble_app_connection());
  
 graphics_context_set_text_color(ctx3, ColorSelect(settings.Text3Color, settings.Text3ColorN));
 graphics_draw_text(ctx3, "z", FontIcon2, BTIconRect, GTextOverflowModeFill,GTextAlignmentCenter, NULL);
  
}

/////////////////////////////////////////
////Init: Handle Settings and Weather////
/////////////////////////////////////////
// Read settings from persistent storage
static void prv_load_settings(){
  // Load the default settings
  prv_default_settings();
  // Read settings from persistent storage, if they exist
  persist_read_data(SETTINGS_KEY, & settings, sizeof(settings));
}
// Save the settings to persistent storage
static void prv_save_settings(){
  persist_write_data(SETTINGS_KEY, & settings, sizeof(settings));
}
// Handle the response from AppMessage
static void prv_inbox_received_handler(DictionaryIterator * iter, void * context){
  s_loop = s_loop + 1;
  if (s_loop == 1){
    //Clean vars
    strcpy(tempstring, "");
    strcpy(condstring, "");
    strcpy(citistring, "");
    strcpy(windstring, "");
    strcpy(sunsetstring, " ");
  }
  // Background Color
  Tuple * bg1_color_t = dict_find(iter, MESSAGE_KEY_Back1Color);
  if (bg1_color_t){
    settings.Back1Color = GColorFromHEX(bg1_color_t-> value -> int32);
  }
  Tuple * nbg1_color_t = dict_find(iter, MESSAGE_KEY_Back1ColorN);
  if (nbg1_color_t){
    settings.Back1ColorN = GColorFromHEX(nbg1_color_t-> value -> int32);
  }
  Tuple * tx1_color_t = dict_find(iter, MESSAGE_KEY_Text1Color);
  if (tx1_color_t){
    settings.Text1Color = GColorFromHEX(tx1_color_t-> value -> int32);
  }
  Tuple * ntx1_color_t = dict_find(iter, MESSAGE_KEY_Text1ColorN);
  if (ntx1_color_t){
    settings.Text1ColorN = GColorFromHEX(ntx1_color_t-> value -> int32);
  }
  ///////////////////////////////
  Tuple * hr_color_t = dict_find(iter, MESSAGE_KEY_HourColor);
  if (hr_color_t){
    settings.HourColor = GColorFromHEX(hr_color_t-> value -> int32);
  }
  Tuple * nthr_color_t = dict_find(iter, MESSAGE_KEY_HourColorN);
  if (nthr_color_t){
    settings.HourColorN = GColorFromHEX(nthr_color_t-> value -> int32);
  }
  Tuple * min_color_t = dict_find(iter, MESSAGE_KEY_MinColor);
  if (min_color_t){
    settings.MinColor = GColorFromHEX(min_color_t-> value -> int32);
  }
  Tuple * ntmin_color_t = dict_find(iter, MESSAGE_KEY_MinColorN);
  if (ntmin_color_t){
    settings.MinColorN = GColorFromHEX(ntmin_color_t-> value -> int32);
  }
  ///////////////////////////////
  Tuple * tx2_color_t = dict_find(iter, MESSAGE_KEY_Text2Color);
  if (tx2_color_t){
    settings.Text2Color = GColorFromHEX(tx2_color_t-> value -> int32);
  }
  Tuple * ntx2_color_t = dict_find(iter, MESSAGE_KEY_Text2ColorN);
  if (ntx2_color_t){
    settings.Text2ColorN = GColorFromHEX(ntx2_color_t-> value -> int32);
  }
   Tuple * tx3_color_t = dict_find(iter, MESSAGE_KEY_Text3Color);
  if (tx3_color_t){
    settings.Text3Color = GColorFromHEX(tx3_color_t-> value -> int32);
  }
  Tuple * ntx3_color_t = dict_find(iter, MESSAGE_KEY_Text3ColorN);
  if (ntx3_color_t){
    settings.Text3ColorN = GColorFromHEX(ntx3_color_t-> value -> int32);
  }
   //Control of data from http
  // Weather Cond
  Tuple * wcond_t = dict_find(iter, MESSAGE_KEY_WeatherCond);
  if (wcond_t){
    snprintf(condstring, sizeof(condstring), "%s", wcond_t -> value -> cstring);
  }
  // Weather Temp
  Tuple * wtemp_t = dict_find(iter, MESSAGE_KEY_WeatherTemp);
  if (wtemp_t){
    snprintf(tempstring, sizeof(tempstring), "%s", wtemp_t -> value -> cstring);
  }
   Tuple * wwind_t = dict_find(iter, MESSAGE_KEY_WeatherWind);
  if (wwind_t){
    snprintf(windstring, sizeof(windstring), "%s", wwind_t -> value -> cstring);
  }
  //Hour Sunrise and Sunset
  Tuple * sunrise_t = dict_find(iter, MESSAGE_KEY_HourSunrise);
  if (sunrise_t){
    HourSunrise = (int) sunrise_t -> value -> int32;
  }
  Tuple * sunset_t = dict_find(iter, MESSAGE_KEY_HourSunset);
  if (sunset_t){
    HourSunset = (int) sunset_t -> value -> int32;
  }
  Tuple * sunset_dt = dict_find(iter, MESSAGE_KEY_WEATHER_SUNSET_KEY);
  if (sunset_dt){
     snprintf(sunsetstring, sizeof(sunsetstring), "%s", sunset_dt -> value -> cstring);
  }
  // Location
  Tuple * neigh_t = dict_find(iter, MESSAGE_KEY_NameLocation);
  if (neigh_t){
    snprintf(citistring, sizeof(citistring), "%s", neigh_t -> value -> cstring);
  }
  //Control of data gathered for http
  APP_LOG(APP_LOG_LEVEL_DEBUG, "After loop %d temp is %s Cond is %s and City is %s and Wind is %s", s_loop, tempstring, condstring, citistring, windstring);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sunrise is %d Sunset is %d SunsetDisplay is %s", HourSunrise,HourSunset,sunsetstring);
  if (strcmp(tempstring, "") != 0 && strcmp(condstring, "") != 0 && strcmp(citistring, "")){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "GPS fully working at loop %d", s_loop);
    GPSOn = true;
  } else{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Missing info at loop %d, GPS false", s_loop);
    GPSOn = false;
  }
  //End data gathered
  // Get display handlers
  Tuple * frequpdate = dict_find(iter, MESSAGE_KEY_UpSlider);
  if (frequpdate){
    settings.UpSlider = (int) frequpdate -> value -> int32;
    //Restart the counter
    s_countdown = settings.UpSlider;
  }
  Tuple * disntheme_t = dict_find(iter, MESSAGE_KEY_NightTheme);
  if (disntheme_t){
    if (disntheme_t -> value -> int32 == 0){
      settings.NightTheme = false;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "NTHeme off");
    } else settings.NightTheme = true;
  } 
  //Update colors
  layer_mark_dirty(s_canvas_to_be_rotated);
  layer_mark_dirty(s_canvas);
  layer_mark_dirty(s_canvas_sunset_icon);
  layer_mark_dirty(s_canvas_bt_icon);
  // Save the new settings to persistent storage
  prv_save_settings();
}



//Load main layer
static void window_load(Window * window){
  Layer * window_layer = window_get_root_layer(window);
  GRect bounds4 = layer_get_bounds(window_layer);
  s_canvas_to_be_rotated = layer_create(bounds4);
  layer_set_update_proc(s_canvas_to_be_rotated, layer_update_proc_pre_rotate);
  layer_add_child(window_layer, s_canvas_to_be_rotated);
  
//  effect_layer = layer_create(bounds);
  
  effect_layer_2 = effect_layer_create(PBL_IF_ROUND_ELSE(
      GRect(0,0,180,180),
      GRect(0, 0,144,168 )));
  effect_layer_add_effect(effect_layer_2, effect_rotate_90_degrees, (void *)true);
 // layer_set_update_proc(effect_layer_2, layer_update_proc_rotate);
  layer_add_child(window_get_root_layer(s_window), effect_layer_get_layer(effect_layer_2));
 
  
  s_canvas = layer_create(bounds4);
    layer_set_update_proc(s_canvas, layer_update_proc);
    layer_add_child(window_layer, s_canvas);
  
  //blur hour and minute
    effect_layer_3 = effect_layer_create(PBL_IF_ROUND_ELSE(
      GRect(0,0,180,180),
      GRect(0, 0,144,168 )));
  effect_layer_add_effect(effect_layer_3, effect_blur,(void *)(0));
 // layer_set_update_proc(effect_layer_2, layer_update_proc_rotate);
  layer_add_child(s_canvas, effect_layer_get_layer(effect_layer_3));
 //   layer_add_child(window_get_root_layer(s_window), effect_layer_get_layer(effect_layer_3));
  
  s_canvas_complications = layer_create(bounds4);
    layer_set_update_proc (s_canvas_complications, layer_update_proc_complications);
    layer_add_child(window_layer, s_canvas_complications);
  
  s_canvas_sunset_icon = layer_create(bounds4);
    layer_set_update_proc (s_canvas_sunset_icon, layer_update_proc_sunset);
    layer_add_child(window_layer, s_canvas_sunset_icon);
  
  s_canvas_bt_icon = layer_create(bounds4);
    layer_set_update_proc (s_canvas_bt_icon, layer_update_proc_bt);
    layer_add_child(window_layer, s_canvas_bt_icon);
  
  s_step_layer = text_layer_create (PBL_IF_ROUND_ELSE(
    GRect((bounds4.size.w/2)+12, 36, bounds4.size.w, 20),  
    GRect((bounds4.size.w/2), 38, (bounds4.size.w/2), 20)));
    text_layer_set_background_color(s_step_layer, GColorClear);
    text_layer_set_font(s_step_layer,
                      FontSteps);
            //          fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_step_layer, (PBL_IF_ROUND_ELSE(GTextAlignmentLeft, GTextAlignmentRight)));
    layer_add_child(window_layer, text_layer_get_layer(s_step_layer));

}


static void window_unload(Window * window){
  layer_destroy(s_canvas_to_be_rotated);
  effect_layer_destroy(effect_layer_2);	
  effect_layer_destroy(effect_layer_3);	
  text_layer_destroy    (s_step_layer);
  layer_destroy(s_canvas);
  layer_destroy(s_canvas_complications);
  layer_destroy(s_canvas_sunset_icon);
  layer_destroy(s_canvas_bt_icon);
  if (effect_layer != NULL) {
	  effect_layer_destroy(effect_layer);
  }
  window_destroy(s_window);
  fonts_unload_custom_font(FontHour);
  fonts_unload_custom_font(FontIcon);
  fonts_unload_custom_font(FontIcon2);
}
void main_window_push(){
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  
//   update_time();
}
void main_window_update(int hours, int minutes, int weekday, int day){
  s_hours = hours;
  s_minutes = minutes;
  s_day = day;
  s_weekday = weekday;
  
  layer_mark_dirty(s_canvas);
  layer_mark_dirty(s_canvas_to_be_rotated);
  layer_mark_dirty(s_canvas_sunset_icon);
  layer_mark_dirty(s_canvas_bt_icon);
    
}

static void tick_handler(struct tm * time_now, TimeUnits changed){

  main_window_update(time_now -> tm_hour, time_now -> tm_min, time_now -> tm_wday, time_now -> tm_mday);
  //update_time();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Tick at %d", time_now -> tm_min);
  s_loop = 0;
  if (s_countdown == 0){
    //Reset
    s_countdown = settings.UpSlider;
  } else{
    s_countdown = s_countdown - 1;
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Countdown to update %d", s_countdown);
  // Evaluate if is day or night
  int nowthehouris = s_hours * 100 + s_minutes;
  if (HourSunrise <= nowthehouris && nowthehouris <= HourSunset){
    IsNightNow = false;
  } else{
    IsNightNow = true;
  }
  // Extra catch on sunrise and sunset
  if (nowthehouris == HourSunrise || nowthehouris == HourSunset){
    s_countdown = 1;
  }
  if (GPSOn && settings.NightTheme){
    //Extra catch around 1159 to gather information of today
    if (nowthehouris == 1159 && s_countdown > 5){
      s_countdown = 1;
    };
    // Change Color of background
    layer_mark_dirty(s_canvas_to_be_rotated);
    
  }
  // Get weather update every requested minutes and extra request 5 minutes earlier
  if (s_countdown == 0 || s_countdown == 5){
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Update weather at %d", time_now -> tm_min);
      request_watchjs();    
  }
  //If GPS was off request weather every 15 minutes
  if (!GPSOn){
      if (settings.UpSlider > 15){
        if (s_countdown % 15 == 0){
          APP_LOG(APP_LOG_LEVEL_DEBUG, "Attempt to request GPS on %d", time_now -> tm_min);
          request_watchjs();
        }
      }
    }  
 }

static void init(){
  color_loser = GColorRed;
  color_winner = GColorJaegerGreen;
  
  prv_load_settings();
  // Listen for AppMessages
  //Starting loop at 0
  s_loop = 0;
  s_countdown = settings.UpSlider;
  //Clean vars
  strcpy(tempstring, "");
  strcpy(condstring, "");
  strcpy(citistring, "");
  strcpy(windstring,"");
  strcpy(sunsetstring, " ");
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  s_hours=t->tm_hour;
  s_minutes=t->tm_min;
  s_day=t->tm_mday;
  s_weekday=t->tm_wday;
  //Register and open
  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(256, 256);
  // Load Fonts
  FontHour = fonts_load_custom_font(resource_get_handle(
      PBL_IF_ROUND_ELSE(RESOURCE_ID_FONT_DINCON_BOLD_86,
              PBL_IF_MICROPHONE_ELSE(RESOURCE_ID_FONT_DINCON_BOLD_90,RESOURCE_ID_FONT_DINCON_BOLD_82))));
  FontDate = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  FontIcon = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WEATHERICONS_12));
  FontIcon2 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DRIPICONS_16));
  FontWeather = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  FontSteps = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  main_window_push();
  // Register with Event Services
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  if(step_data_is_available()){
    health_service_events_subscribe(health_handler,NULL);
  }
  connection_service_subscribe((ConnectionHandlers){
    .pebble_app_connection_handler = bluetooth_vibe_icon
  });
  bluetooth_vibe_icon(connection_service_peek_pebble_app_connection());
 // handle_battery(battery_state_service_peek());
}
static void deinit(){
  tick_timer_service_unsubscribe();
  app_message_deregister_callbacks(); //Destroy the callbacks for clean up
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  health_service_events_unsubscribe();
}
int main(){
  init();
  app_event_loop();
  deinit();
}