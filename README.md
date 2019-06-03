
# High Frequency Analog Sampler for the m5stickC ESP32 dev board. 
<img src="https://m5stack.oss-cn-shenzhen.aliyuncs.com/image/m5-docs_content/core/m5stickc_01.png" alt="M5StickC_01" width="350" height="350">

## Description

Designed for ad8232 analog conditioner input i.e for ECG but could be used as a general purpose scope. Tested upto 500Khz Sampling rate

## Todo
- [ ] FFT Implementation
- [X] Button Support (Pause Display)
- [X] ADC or I2S mode
- [X] Perf Analysis
- [ ] Better Interupt timers
- [ ] ADC or I2S mode toggle without reflash
- [ ] Selectable Sample Rate without reflash
- [ ] Selectable Average/Sum without reflash
- [ ] Implement Web Server
- [ ] Realtime HTML5 Scope Implementation

## Pinout
<img src="https://github.com/aenertia/m5stick-scope-ecg/blob/master/pinout.jpg" alt="Pinout">

## Working Pictures

<img src="https://github.com/aenertia/m5stick-scope-ecg/blob/master/working.gif" alt="Working Movie">

### Destructive Disasembly

I took the M5StickC apart as M5Stack haven't published any photos on internals etc which is anoying and no-one else seems to have either. 

** Caution **

It turns out the Pinout PCB insert that is wired to the main PCB (and the Aerial) is pretty much going to break if you don't first cut (or expand) the case gap around the Dupont Female header at the top of the unit. If you don't want to break your m5StickC I suggest cutting that area first. 

Also there is a magnet in the back which is undocumented which means it will stick to a fridge etc which is kinda neat. 

I Plan to wire a direction switch or Analog to the 'A' button on my unit (why they didn't use a step ladder button / DPAD is beyond my comprehension).
<img src="https://github.com/aenertia/m5stick-scope-ecg/blob/master/pcb/decon_pcb1.jpg">
<img src="https://github.com/aenertia/m5stick-scope-ecg/blob/master/pcb/decon_pcb2.jpg">
<img src="https://github.com/aenertia/m5stick-scope-ecg/blob/master/pcb/decon_pcb3.jpg">
<img src="https://github.com/aenertia/m5stick-scope-ecg/blob/master/pcb/decon_pcb4.jpg">
<img src="https://github.com/aenertia/m5stick-scope-ecg/blob/master/pcb/decon_pcb5.jpg">
<img src="https://github.com/aenertia/m5stick-scope-ecg/blob/master/pcb/decon_pcb6.jpg">
<img src="https://github.com/aenertia/m5stick-scope-ecg/blob/master/pcb/decon_pcb7.jpg">
<img src="https://github.com/aenertia/m5stick-scope-ecg/blob/master/pcb/decon_pcb8.jpg">
<img src="https://github.com/aenertia/m5stick-scope-ecg/blob/master/pcb/decon_pcb9.jpg">
<img src="https://github.com/aenertia/m5stick-scope-ecg/blob/master/pcb/decon_pcb10.jpg">
<img src="https://github.com/aenertia/m5stick-scope-ecg/blob/master/pcb/decon_pcb11.jpg">




