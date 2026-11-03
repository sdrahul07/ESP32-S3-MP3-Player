#include <Arduino.h>
#include <driver/i2s_std.h>
#include <math.h>

// PCM5102A LINE OUTPUT ONLY: use powered speakers or an external headphone amp.
// BCK -> GPIO4, LCK/LRCK -> GPIO5, DIN -> GPIO6, SCK -> GND, GND -> GND.
// Power VIN according to the breakout's regulator specification.
// FMT LOW (I2S), XSMT HIGH (unmuted), DEMP LOW. Existing straps may set these.
i2s_chan_handle_t toneTx=nullptr;
constexpr uint32_t RATE=44100;
int32_t pcm[256*2];
double leftPhase=0,rightPhase=0;

void setup(){
  Serial.begin(115200);delay(1000);
  Serial.println("\nPCM5102A tone test: left 440 Hz / right 660 Hz, low level");
  i2s_chan_config_t ch=I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0,I2S_ROLE_MASTER);
  ch.dma_desc_num=6;ch.dma_frame_num=256;ch.auto_clear=true;
  esp_err_t result=i2s_new_channel(&ch,&toneTx,nullptr);
  if(result!=ESP_OK){Serial.printf("I2S allocation failed: %s\n",esp_err_to_name(result));while(true)delay(100);}
  i2s_std_config_t cfg={};
  cfg.clk_cfg=I2S_STD_CLK_DEFAULT_CONFIG(RATE);
  cfg.slot_cfg=I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,I2S_SLOT_MODE_STEREO);
  cfg.gpio_cfg.mclk=I2S_GPIO_UNUSED;
  cfg.gpio_cfg.bclk=GPIO_NUM_4;cfg.gpio_cfg.ws=GPIO_NUM_5;
  cfg.gpio_cfg.dout=GPIO_NUM_6;cfg.gpio_cfg.din=I2S_GPIO_UNUSED;
  result=i2s_channel_init_std_mode(toneTx,&cfg);
  if(result==ESP_OK)result=i2s_channel_enable(toneTx);
  if(result!=ESP_OK){Serial.printf("I2S setup failed: %s\n",esp_err_to_name(result));while(true)delay(100);}
  Serial.println("I2S transmitting. This does not electronically confirm DAC detection.");
}

void loop(){
  const double tau=6.283185307179586;
  for(int i=0;i<256;i++){
    pcm[2*i]=int32_t(sin(leftPhase)*64424509.0); // 3% full scale.
    pcm[2*i+1]=int32_t(sin(rightPhase)*64424509.0);
    leftPhase+=tau*440/RATE;rightPhase+=tau*660/RATE;
    if(leftPhase>=tau)leftPhase-=tau;if(rightPhase>=tau)rightPhase-=tau;
  }
  size_t offset=0;
  while(offset<sizeof(pcm)){
    size_t written=0;esp_err_t result=i2s_channel_write(toneTx,reinterpret_cast<uint8_t*>(pcm)+offset,sizeof(pcm)-offset,&written,200);
    if(result!=ESP_OK||!written){Serial.printf("I2S write failed: %s\n",esp_err_to_name(result));delay(100);break;}
    offset+=written;
  }
  delay(1);
}
