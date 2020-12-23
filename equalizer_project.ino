//Примечание: Serial.println("Button _ is pressed"); - проверка работоспособности кнопок через монитор порта

#define LOW_PASS 30        // нижний порог чувствительности шумов (нет скачков при отсутствии звука)
#define DEF_GAIN 80       // максимальный порог по умолчанию
#define FHT_N 256          // количество входных отсчетов
byte posOffset[16] = {2, 3, 4, 6, 8, 10, 12, 14, 16, 20, 25, 30, 35, 60, 80, 100}; //массив тонов
#define LOG_OUT 1     

#include <FHT.h>          // преобразование Хартли
#include <Adafruit_PCD8544.h>       //библиотека дисплея
Adafruit_PCD8544 display = Adafruit_PCD8544(9, 10, 11, 12, 13);
#include <EEPROM.h>       //библиотека для сохранения настроек

//пин микрофона
#define MIC_PIN A0
//пины джойстика
#define ANALOG_Y_pin A1
#define RIGHT_pin 3
#define LEFT_pin 5
#define UP_pin 2
#define DOWN_pin 4
#define BUTTON_E 6
#define BUTTON_G 8

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

byte gain = DEF_GAIN;     // усиление
byte maxValue, maxValue_f;

uint8_t columns = 16;
uint8_t selectedOption = 0;
uint8_t menuArea = 0;
uint8_t contrast = 70;
uint8_t col = 5;
uint8_t width = 4;

void AnalyzeAudio()       //FHT
{
  for (int i = 0 ; i < FHT_N ; i++) 
  {
    int sample = analogRead(MIC_PIN);
    fht_input[i] = sample;        
  }
  fht_window();       //функция-окно, повышающая частотное разрешение
  fht_reorder();      //реорганизовываем данные перед запуском FHT
  fht_run();        //обрабатываем данные в FHT
  fht_mag_log();      //извлекаем данные, обработанные FHT
}

void Spectrum()     //вывод
{
  for (int pos = 0; pos < columns; pos++) 
  {   
    //ищем максимальную амплитуду в диапазоне
    if (fht_log_out[posOffset[pos]] > maxValue) maxValue = fht_log_out[posOffset[pos]];
    
    //преобразуем величины
    int posLevel = map(fht_log_out[posOffset[pos]], LOW_PASS, gain, 0, 47);
    posLevel = constrain(posLevel, 0, 47);

    //выводим спектр на экран
    display.fillRect(1+col*pos, 47, width, -posLevel, 1);     //заполнение столбца
    display.display();
  }
  //display.clearDisplay();

  if(analogRead(ANALOG_Y_pin)==0)     //Y=0 выход из эквалайзера
  {
    Serial.println("Button Y is pressed");
    delay(200);        
    menuArea=0;
    selectedOption=0;
    Menu();
  }
}

void Menu()     //главное меню
{
  while (true)
  {
    if ((digitalRead(UP_pin)==LOW)||(digitalRead(DOWN_pin)==LOW))     //перемещение по меню
    {
      delay(200);
      if (selectedOption==0)
      {
        selectedOption = 1;
      }
      else selectedOption = 0;
      Serial.println(selectedOption);
    }
    display.clearDisplay();
    if (menuArea==0)
    {
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(25,10);
      display.println("START");
      display.setCursor(18,30);
      display.println("SETTINGS");
      display.drawLine(18, 17+(selectedOption*20), display.width()-18,  17+(selectedOption*20), BLACK);//x_start,y_start,x_end,y_end,color
      display.display();

      if (digitalRead(BUTTON_G)==LOW)
      {
        Serial.println("Button G is pressed");
        delay(200);
        display.clearDisplay();
        if(selectedOption==0)
        {
          menuArea = 1;
        }
        else
        {
          menuArea = 2;
        }
      }
    }
    else if (menuArea==1)
    {
      display.clearDisplay();
      AnalyzeAudio();
      Spectrum();
    }
    else if (menuArea==2)
    {
      display.clearDisplay();
      selectedOption = 0;
      Settings();
    }
  }
}

void Settings()     //настройки
{
  while(true)
  {
    if (menuArea==2)
    {
      if ((digitalRead(UP_pin)==LOW)||(digitalRead(DOWN_pin)==LOW))     //перемещение по меню
      {
        delay(200);
        if (selectedOption==0)
        {
          selectedOption = 1;
        }
        else selectedOption = 0;
        Serial.println(selectedOption);
      }
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK);
    display.setCursor(0,5);
    display.print("Contrast: ");
    display.println(contrast);
    display.print("Columns: ");
    display.println(columns);
    //полоса под выбранным
    display.drawLine(0, 12+(selectedOption*8), display.width()-38,  12+(selectedOption*8), BLACK);//x_start,y_start,x_end,y_end,color 
    display.display();

    //изменяем настройку
    if((digitalRead(RIGHT_pin)==LOW)||(digitalRead(LEFT_pin)==LOW))
    {
      //КОНТРАСТНОСТЬ
      if(selectedOption==0)
      {
        if(digitalRead(RIGHT_pin)==LOW)     //увеличить контрастность
        {
          delay(200);
          if(contrast<=124)
          {
            contrast+=2;
          }
          display.setContrast(contrast);
        }
        else if(digitalRead(LEFT_pin)==LOW)     //уменьшить контрастность
        {
          delay(200);
          if(contrast>=0)
          {
            contrast-=2;
          }
          display.setContrast(contrast);
        }
        EEPROM.put(0, contrast);      //сохранить в памяти устройства
      }
      //КОЛИЧЕСТВО СТОЛБИКОВ
      if(selectedOption==1)
      {
        if(digitalRead(RIGHT_pin)==LOW)     //увеличить количество столбиков
        {
          delay(200);
          if(columns<=15)
          {
            columns+=4;
            width = (84/columns-1);
            col = width+1; 
          }
        }
        else if(digitalRead(LEFT_pin)==LOW)     //уменьшить количество столбиков
        {
          delay(200);
          if(columns>=9)
          {
            columns-=4;
            width = (84/columns-1);
            col = width+1; 
          }
        }
        EEPROM.put(9, columns);      //сохранить в памяти устройства
      }      
    }

    if(analogRead(ANALOG_Y_pin)==0)       //выход из настроек
    {
      Serial.println("Button Y is pressed");
      delay(200);
      menuArea=0;
      selectedOption=0;
      Menu();
    }
  }
}

void SetMem()
    {
      contrast = 70;
      columns = 16;
      if (EEPROM.read(0) >= 0 && EEPROM.read(0) <= 124)
        EEPROM.get(0,contrast);
      if (EEPROM.read(9) >= 8 && EEPROM.read(9) <= 16)
        EEPROM.get(18,columns);
    }

void setup() {
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  sbi(ADCSRA, ADPS0);

  Serial.begin(9600);

  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);     
  display.begin();
  display.clearDisplay();
  display.setContrast(60);
}

void loop() 
{
  SetMem();
  
  Menu();
  
  //AnalyzeAudio();   // функция FHT, забивает массив fht_log_out[] величинами по спектру

  //Spectrum();     //преобразует величины и выводит спектр на экран
}
