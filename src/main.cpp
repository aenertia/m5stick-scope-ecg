#include <Arduino.h>
#include <M5StickC.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
//i2smode
#include <driver/i2s.h>

#define ADC_INPUT ADC1_CHANNEL_0 //pin 36
//refvoltage and attenuation
#define DEFAULT_VREF 1121 //get this from your boards efuse $espefuse.py --port /dev/ttyUSB0 adc_info
//number of samples to take for averaging 5@22050 seems good for smoothing noise - used differently for i2s vs analog for analog is literally buffer of samples for i2s it store of sums
#define SAMPLES 32000 //250ms to fill @40000Hz SAMPLE_RATE
//i2s sample rate if in i2smode
//#define I2S_SAMPLE_RATE 320000
//#define I2S_SAMPLE_RATE 160000 //with appl 10*mclk ~75ms for 16000 samples
//#define I2S_SAMPLE_RATE 80000 //with appl set to 10*mclk this gets me around 100ms for 16000 samples
//#define I2S_SAMPLE_RATE 125125 //2000ms
//#define I2S_SAMPLE_RATE 78125 //1383ms, 4166ms with appl clock
//#define I2S_SAMPLE_RATE 121000 //1926ms
//#define I2S_SAMPLE_RATE 96000 //1450ms
//#define I2S_SAMPLE_RATE 352800 //7502ms
//#define I2S_SAMPLE_RATE 126000 //around 2003ms to sample this
//#define I2S_SAMPLE_RATE 60000 //around 1000ms to sample this
//#define I2S_SAMPLE_RATE 240000 //4365ms
//#define I2S_SAMPLE_RATE 50000 //1122ms
//#define I2S_SAMPLE_RATE 48000 //1100ms
//#define I2S_SAMPLE_RATE 25000 //1008ms
//#define I2S_SAMPLE_RATE 126125 //2018ms
//#define I2S_SAMPLE_RATE 22050 //473ms with appl clock, 1010ms without
//#define I2S_SAMPLE_RATE 31050 //843ms with appl
#define I2S_SAMPLE_RATE 80000
//#define I2S_SAMPLE_RATE 35050
//#define I2S_SAMPLE_RATE 1000000 // doable with fixed APPL
//Characterize ADC at particular atten
esp_adc_cal_characteristics_t *adc_chars = new esp_adc_cal_characteristics_t;

//esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
//esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
/* wifi host id and password */
const char* ssid = "kainga-atawhai";
const char* password = "q1w2e3o0";

//globals
uint16_t buffer[SAMPLES] = {0};
uint16_t bufferi2s[2] = {0};
uint16_t adc_reading; //average of SAMPLES
uint32_t read_counter = 0;
uint64_t read_sum = 0;
uint32_t period = 0;
uint32_t periodS = 0;
uint32_t lastperiod = 0;
uint32_t millistimer = millis();
uint32_t sampletimer = millis();
float frequency;
float beatspermin = 60;
//uint32_t sps; //samples per second
//uint32_t sps_last; //samples per second


// The 4 high bits are the channel, and the data is inverted for i2s mode
uint16_t offset = (int)ADC_INPUT * 0x1000 + 0xFFF;

// Lcd array
uint16_t oldx[160];
uint16_t oldy[160];
uint16_t oldyA[160];


///////////
//Functions
///////////

//setup I2S

void i2sInit()
{
   i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate =  I2S_SAMPLE_RATE,              // The format of the signal using ADC_BUILT_IN
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 128,
    //.use_apll = false,
    .use_apll = true,
    //.tx_desc_auto_clear = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 640000
   };
   i2s_set_clk(I2S_NUM_0, I2S_SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
   i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
   i2s_set_adc_mode(ADC_UNIT_1, ADC_INPUT);
   i2s_adc_enable(I2S_NUM_0);
}

void calcbpm() {
  if (period != lastperiod && period > 0) {
    frequency =  I2S_SAMPLE_RATE / (double)period ; //Number of Periods in a Second
    if(frequency * 60 > 20 && frequency * 60 < 200) { // supress unrealistic Data
  beatspermin = frequency * 60;
  lastperiod = period;
  }
 }
}


// This is the Reader Task Pinned to core 1

void reader(void *pvParameters) {

    // The 4 high bits are the channel, and the data is inverted
    //uint16_t offset = (int)ADC_INPUT * 0x1000 + 0xFFF;
    while(1){
        buffer[read_counter] = analogRead(36);
        if (read_counter < SAMPLES ) {
            read_sum +=  buffer[read_counter];
            read_counter++;
            if (buffer[read_counter-1] < adc_reading * 1.5 &&  buffer[read_counter] >= adc_reading * 1.5) {
                period = millis() - millistimer; //get period from current timer value
                millistimer = millis();//reset timer
            }

        }
        if (read_counter == SAMPLES) {
            adc_reading = read_sum / SAMPLES ;
            // Serial.printf("millis: ");

            //debug
            //Serial.println(millis()-sampletimer);
            //sampletimer = millis(); //reset sampletimer
            read_counter = 0;
            read_sum = 0;
        }

    }

}

// This is the Reader Task but using I2S

void i2sreader(void *pvParameters) {

    // The 4 high bits are the channel, and the data is inverted
    //uint16_t offset = (int)ADC_INPUT * 0x1000 + 0xFFF;
    size_t bytes_read;
    while(1){
        i2s_read(I2S_NUM_0, &bufferi2s, sizeof(bufferi2s), &bytes_read, 15);
        if (bytes_read == sizeof(bufferi2s)) {
            read_sum +=  bufferi2s[0];
            read_sum +=  bufferi2s[1];
            buffer[read_counter] = bufferi2s[0];
            buffer[read_counter+1] = bufferi2s[1];
            read_counter=read_counter+2;
            if (offset - (buffer[read_counter-1] + buffer[read_counter-2] + buffer[read_counter-3] + buffer[read_counter-4] + buffer[read_counter-5]/5)< offset - adc_reading * 1.5 && offset - (bufferi2s[0] + bufferi2s[1] / 2) >= adc_reading * 1.5) {
                period = millis() - millistimer; //get period from current timer value
                millistimer = millis();//reset timer
            }

        }
        if (read_counter == SAMPLES) {
            adc_reading = read_sum / SAMPLES / 2 ;

            //debug
            //Serial.println(millis()-sampletimer);
            periodS = millis() - sampletimer;
            sampletimer = millis(); //reset sampletimer
            //Serial.printf("millis: ");
            //Serial.printf("%d\n", adc_reading);

            read_counter = 0;
            read_sum = 0;
            i2s_adc_disable(I2S_NUM_0);
            delay(1);
            i2s_adc_enable(I2S_NUM_0);
        }



    }

}


//Pause Button Function

void buttonWait(int buttonPin){
    int buttonState = 0;
    while(1){
        buttonState = digitalRead(buttonPin);
        if (buttonState == HIGH) {
            return;
        }
    }
}

//LCD Waveform Output

void showSignal(){
    int x, y, yA;
    for (int n = 0; n < 160; n++){
        delay(2); //24fps
        x = n;
        y = map(offset - buffer[read_counter], 0, 4095, 10, 80);
        //y = map(adc_reading, 0, 4095, 10, 80);
        //Clear lines ahead
        M5.Lcd.fillRect(x-1,8,4,80,BLACK);
        //clear pixel
        //M5.Lcd.drawPixel(x,y,BLACK);
        //draw
        //M5.Lcd.fillCircle(x,y,2,GREEN);
        //draw pixel
        //M5.Lcd.drawPixel(x,y,YELLOW);
        //clear previous
        //draw line between current and previous
        M5.Lcd.drawLine(oldx[n], oldy[n], x, y, YELLOW);
        for (int d = n; d > 0; d--){
        M5.Lcd.drawLine(oldx[d-1], oldy[d-1], oldx[d], oldy[d], GREEN);
        //M5.Lcd.drawPixel(oldx[n], oldy[n],BLACK);
        //M5.Lcd.drawCircle(oldx[n],oldy[n],2,BLACK);
              }
        oldx[n] = x;
        oldy[n] = y;
        }
     }

//LCD Waveform I2S Mode
void showSignalI2S(){
    int x, y, yA;
    for (int n = 0; n < 160; n++){
        //delay(12); //This delay allows for 2 QRS complexes on LCD at the Same time with enough delay to be useful
        x = n;
        y = map(offset - (buffer[read_counter]+buffer[read_counter-1]/2), 4096, 0, 10, 80);
        yA = map(offset - adc_reading, 4098, 0, 10, 80);
        //Clear lines ahead
        M5.Lcd.fillRect(x-2,8,6,80,BLACK);
        //clear pixel
        //M5.Lcd.drawPixel(x,y,BLACK);
        //draw
        //M5.Lcd.fillCircle(x,y,2,GREEN);
        //draw pixel
        //M5.Lcd.drawPixel(x,y,YELLOW);
        //clear previous
        //draw line between current and previous
        M5.Lcd.drawLine(oldx[n], oldy[n], x, y, YELLOW);
        M5.Lcd.drawLine(oldx[n], oldyA[n], x, yA, RED);
        //fill trace
        for (int d = n; d > 0; d--){
        M5.Lcd.drawLine(oldx[d-1], oldy[d-1], oldx[d], oldy[d], GREEN);
        M5.Lcd.drawLine(oldx[d-1], oldyA[d-1], oldx[d], oldyA[d], RED);
        //M5.Lcd.drawPixel(oldx[n], oldy[n],BLACK);
        //M5.Lcd.drawCircle(oldx[n],oldy[n],2,BLACK);
              }
        oldx[n] = x;
        oldy[n] = y;
        oldyA[n] = yA;
        }
     }



///////////
//Setup Environment
///////////

void setup() {

    M5.begin();
    //set serial high but only after the m5 init which defaults to 115200
    //Serial.begin(1500000);
    //set adc sample bits to 11BIT (native 12bit) - unavailable in i2s mode will sample at 12bit and pad
    analogReadResolution(12);
    //set adc1 attenuation
    adc1_config_channel_atten(ADC1_CHANNEL_0 ,ADC_ATTEN_11db);
    //set vref on adc - unsure if this affects anything when in i2s I2S_MODE_ADC_BUILT_IN
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);

    //begin wifi connect
    WiFi.begin(ssid, password);

    /* indication of connection process is commented to avoid serial pollution */
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        //Serial.print(".");
    }
    // Serial.println("");
    // Serial.print("Connected to ");
    // Serial.println(ssid);
    // Serial.print("IP address: ");
    // Serial.println(WiFi.localIP());

    //Start OLED
    M5.Lcd.setRotation(3);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE, BLACK);
    //
    // Create a task that will read the data
    //xTaskCreatePinnedToCore(reader, "ADC_reader", 2048, NULL, 1, NULL, 1);
    //i2smode
    i2sInit();
    xTaskCreatePinnedToCore(i2sreader, "ADC_reader", 2048, NULL, 1, NULL, 1);
    //xTaskCreate(i2sreader, "ADC_reader", 2048, NULL, 1, NULL);
    //xTaskCreate(showSignalI2Stask, "Display_draw", 2048, NULL, 5, NULL);

}
///////////
//Low Priority main loop
///////////

void loop() {
    //run Lcd
    calcbpm();//
    //showSignal();
    showSignalI2S();
  //print stats to screen
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println(" ");
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print(adc_reading);
  M5.Lcd.print(" BPM:");
  M5.Lcd.print(beatspermin);
  //M5.Lcd.print(" Hz:");
  //M5.Lcd.print(frequency);
  //M5.Lcd.print(" msP:");
  //M5.Lcd.print(period);
  // M5.Lcd.print(" msP:");
  // M5.Lcd.print(periodS);
  //output to serial
  //Serial.printf("%d ", map(buffer[read_counter],4096, 0, 0, 4096));
  //Serial.printf("%d\n", adc_reading);

  //Serial.printf("%d\n", buffer[read_counter]);
  //Serial.println(analogRead(36));
  //invert
  //Serial.printf("%d\n",map(offset - buffer[0],4096, 0, 0, 4096));
}
