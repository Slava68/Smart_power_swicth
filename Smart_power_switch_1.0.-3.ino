#include <EEPROM.h>              // библиотека для сохранения рег.данных пультов в энергонезависимой памяти
#include <RCSwitch.h>            // библиотека для декодирования сигнала с пульта

#define Input 0                  // вход с приёмника радиосигнала, обязательно на INT0 (0) PD2 или INT1 (1) PD3

#define Ch_0 B00111000           // соответствие всех каналов для управления нагрузкой (здесь тоже нужно правильно указать выводы порта)
#define Ch_1 B00001000           // соответствие канала для управления нагрузкой выводу Arduino (3) PD3
#define Ch_2 B00010000           // соответствие канала для управления нагрузкой выводу Arduino (4) PD4
#define Ch_3 B00100000           // соответствие канала для управления нагрузкой выводу Arduino (5) PD5
//#define Ch_4 B01000000         // соответствие канала для управления нагрузкой выводу Arduino (6) PD6

#define prg_mode 0xFF            // код для входа в режим программирования дополнительных функций

unsigned long old_time = 0;      // переменная для регистрации времени
byte count_time = 0;             // переменная для подсчёта времени удержания нажатой кнопки
byte pad_number = 0;             // номер текущего пульта

RCSwitch mySwitch = RCSwitch();  // радиоприёмник, как "объект" mySwitch


//------------------------------ декодирование радио-посылки -------------------
byte Code_Decode() {

  unsigned long  code = mySwitch.getReceivedValue(); //принимаем всю посылку 24 бита.
  unsigned int address = code >> 8;                  // удаляем 8 бит кода кнопки оставляем 16 бит адрес пульта.
  byte Button = code & 0xFF;                         // выделяем из полной посылки только код кнопки.

  if (Verify_Remote (address)) {                     // проверяем пульт по базе известных пультов.
    mySwitch.resetAvailable();                       // сброс буфера приёмника
    return Button;                                   // если пульт зарегистрирован в базе, то возвращаем код нажатой кнопки.
  }
  // если пульт не зарегистрирован, то
  New_Remote(address);                               // вызываем подпрограмму регистрации нового пульта.
  return 0;                                          // возвращаем значение кнопки 0x0 (HEX)
}

// ---------------- регистрация нового пульта ---------------------------
void New_Remote(unsigned int addr) {

  if (count_time == 0 ) {
    old_time = millis();                                             // старт процесса регистрации. Фиксируем время для выполнения процедуры
  }
  count_time++;                                                      // при каждом нажатии кнопки незарегистрированного пульта увеличиваем счётчик
  if ((count_time > 20) && ((millis() - old_time) < 20000) ) {       // если кнопка нажата более 20 раз в течении 20 секунд, то
    for (byte x = 0; x < 6; x++) {                                   // подаём сигнал о начале программирования каналов
      PORTD |= Ch_0;  // три раза мигают все каналы
      delay(300);                                                    // с интервалом 0.3 секунды
      PORTD &= ~Ch_0; // выключаем все каналы
      delay(300);
    }
    delay(1000);                                                     // пауза 1 сек, чтобы успеть отпустить кнопку

    // программирование соответствия каналов кнопкам

    for (byte x = 0; x <= 3; x++) {                                  // можно изменить количество программируемых каналов (сейчас 3)
      Ch_control (x);                                                // включаем программируемый канал
      old_time = 1;
      mySwitch.resetAvailable();                                     // сброс буфера приёмника
      while (old_time != 0) {
        if (mySwitch.available()) {                                  // если что-то появилось в буфере приёмника,то
          EEPROM.write(EEPROM.read(0) * 16 + 3 + x, (mySwitch.getReceivedValue() & 0xFF)); // принимаем всю посылку 24 бита, выделяем из полной посылки только код кнопки.
          Ch_control (x);
          delay(500);                                                 // гасим программируемый канал на 0.5 секунды
          Ch_control (x);
          delay(200);
          Ch_control (x);
          delay(200);                                                 // подтверждающее мигание
          Ch_control (x);
          delay(200);
          Ch_control (x);
          old_time = 0;                                               // выходим из бесконечного цикла
        }
      }
      delay(500);                                                     // задержка перед программированием следующего канала
    }

    EEPROM.write((EEPROM.read(0) * 16 + 1), (addr & 0xFF));            // записываем в EEPROM младший байт адреса нового пульта
    EEPROM.write((EEPROM.read(0) * 16 + 2), (addr >> 8));              // записываем в EEPROM старший байт адреса нового пульта
    EEPROM.write(0, (EEPROM.read(0) + 1));
    old_time = millis();                                               // сбрасываем время ожидания удержания кнопки неизвестного пульта.
    count_time = 0;                                                    // сбрасываем счётчик удержания кнопки неизвестного пульта.
  }

  else if ((count_time < 20) && ((millis() - old_time) > 20000)) {     // если прошло 20 секунд и не выполнено условие нажатия кнопок нового пульта, то
    old_time = millis();                                               // сбрасываем время ожидания удержания кнопки неизвестного пульта.
    count_time = 0;                                                    // сбрасываем счётчик удержания кнопки неизвестного пульта.
    mySwitch.resetAvailable();                                         // сброс буфера приёмника
    return;                                                            // возвращаемся к обычному режиму работы
  }
  mySwitch.resetAvailable();                                           // сброс буфера приёмника
  return;
}

// ----------------- проверка номера пульта по базе данных известных пультов ------------

boolean Verify_Remote (unsigned int address) {                         // проверка номера пульта по базе данных известных пультов

  pad_number =  EEPROM.read(0);                                        // в нулевой ячейке храним количество запрограммированных пультов.
  for (byte i = 0; i <= pad_number; i++) {                             // все зарегистрированные пульты
    unsigned int addr = (EEPROM.read(16 * i + 2) << 8) + EEPROM.read(16 * i + 1); // читаем младший байт первым, читаем старший байт адреса пульта, получаем адрес пульта
    if (address == addr) {                                             // проверяем все известные пульты на совпадение
      pad_number = i;                                                  // при совпадении, устанавливаем глобальную переменную "текущий пульт"
      return 1;                                                        // если нашли совпадение, возвращаем TRUE
    }
  }
  return 0;                                                            // если не нашли совпадений возвращаем FALSE
}

// ----------------- при вызове функции инвертируется состояние управляемого канала -------------

void Ch_control (byte Ch) {                                         // использование этой функции заманчиво, но может быть оптимальнее от неё отказаться

  // попробуем вариант с инверсией каналов
  if      (Ch == 0 && (PIND & Ch_0) != 0)  PORTD &= ~Ch_0;          // если хотя бы один канал включен, выключаем все каналы
  else if (Ch == 0 && (PIND & Ch_0) == 0)  PORTD |= Ch_0;           // если ничего не было включено, то выключаем все каналы

  else if (Ch == 1) PORTD ^= Ch_1;                                  // инвертируем состояние канала номер 1
  else if (Ch == 2) PORTD ^= Ch_2;                                  // инвертируем состояние канала номер 2
  else if (Ch == 3) PORTD ^= Ch_3;                                  // инвертируем состояние канала номер 3
  // else if (Ch == 4) PORTD ^= Ch_4;                                  // инвертируем состояние канала номер 4

  return;
}

//------------- программирование дополнительных функций  ------------------
void Programm_mode() {                                               // режим программирования дополнительных функций устройства

  boolean F_erase = 1;                                               // устанавливаем флаг готовности к удалению информации в EEPROM.
  byte erase_time = 50;                                              // устанавливаем счётчик для  выхода из этого режима
  while (erase_time != 0) {                                          // до тех пор, пока он не станет равным "0"
    mySwitch.resetAvailable();                                       // очищаем буфер приемника
    if (mySwitch.available()) {                                      // читаем коды кнопок
      byte prg = Code_Decode();                                      // отправляем посылку на расшифровку, получаем код следующей команды


      if (prg == prg_mode) {                                         // если код кнопки "0xFF", то продолжаем
        Ch_control(0);                                               // при каждом получении команды переключаем состояние всех каналов
        erase_time--;                                                // обратный отсчёт до обнуления памяти
        delay(100);
      }
      // -------------- блок программирования одной функции ----------------
      else if (prg == 0x03) {                                        // если код кнопки не "0xFF",  0x03 - "все каналы
        F_erase = 0;                                                 // то сразу отменяем процедуру очищения памяти и переходим в режим программирования.
        mySwitch.resetAvailable();                                   // очищаем буфер приемника
        while (1) {                                                  // бесконечный цикл ожидания кнопки канала, включенного при подаче питания
          for (byte x = 3; x < 7; x++) {                             // управляющие биты каналов. Мигаем "бегущий огонь" (забавно будет на одноканальном устройстве :-))
            PORTD &= ~B01111000;
            PORTD |= (1 << x);                                       // просто мигаем каналами для красоты режима программирования
            delay(300);
          }

          if (mySwitch.available()) {                                // читаем коды кнопок
            byte prg = Code_Decode();                                // отправляем посылку на расшифровку, получаем код следующей команды

            for (byte i = 1 ; i < 5; i++) {                          // перебираем все соответствия для текущего пульта. Может быть одна команда для нескольких каналов
              if (prg == EEPROM.read(3 + i + pad_number * 16)) {     // Ячейки 250 - 254 определяют начальное состояние после подачи питания. Состояние меняется на противоположное.
                if (EEPROM.read(250 + i)) EEPROM.write(250 + i, 0);  // если ячейка не пустая, то пишем в неё "0"
                else if (!EEPROM.read(250 + i)) EEPROM.write(250 + i, 0xFF); // если пустая, то пишем туда "0xFF"
                PORTD &= ~Ch_0;                                      // выключаем все каналы
                for (byte x = 0; x < 10; x++) {                      // подаём сигнал о окончании программирования канала. 5 раз мигает  канал
                  Ch_control(i);
                  delay(100);                                        // с интервалом 0.1 секунды
                }
                PORTD &= ~Ch_0;                                      // выключаем все каналы
                return;
              }
            }
          }
        } // бесконечный цикл  ожидания кнопки

      } // конец блока программирования одной функции , можно далее делать следующий блок.

    } // mySwitch.available

  } //while

  if (F_erase == 1) {
    delay(2000);
    EEPROM.write(0, 0);                                               // устанавливаем число зарегистрированных пультов равным "0" в нулевой ячейке заполняем всю остальную EEPROM символом "FF"
    for (byte x = 1; x < 250; x++) EEPROM.write(x, 0xFF);             // затираем данные всех пультов, оставляя без изменения "состояние каналов при инициализации".

    for (byte x = 0; x < 10; x++) {                                   // подаём сигнал о окончании программирования канала. 5 раз мигает  канал
      Ch_control(0);
      delay(100);                                                     // с интервалом 0.1 секунды
    }
    PORTD &= ~Ch_0;                                                   // выключаем все каналы
    return;
  }
  return;
}


//----------------------------------- начальная инициализация системы ----------------------------
void setup() {

  mySwitch.enableReceive(Input);
  if (EEPROM.read(0) == 255)  {
    EEPROM.write(0, 0);                                              // первоначальная инициализация новой микросхемы. Выполняется всего 1 раз (или при наполнении до 15 пультов)
    EEPROM.write(250, 0);
    EEPROM.write(251, 0);
    EEPROM.write(252, 0);                                            // Ячейки 250 - 254 определяют начальное состояние после подачи питания.
    EEPROM.write(253, 0);
    EEPROM.write(254, 0);                                            // 0 - выключено , 0xFF -включено.
  }
  DDRD |= Ch_0;                                                      // устанавливаем все используемые выводы в режим "выход"
  PORTD |= B11111111 ^ Ch_0;
  PORTB |= B11111111;                                                // Для неиспользуемых выводов рекоммендуется устанавливать этот режим, это приведёт к снижению энергопотребления и повышению надёжности
  PORTC |= B11111111;

  if (EEPROM.read(0) == 0 ) {                                        // если не запрограммировано ни одного пульта
    PORTD |= Ch_0;                                                   // включаем все каналы для проверки работоспособности при начальной инициализации,
    delay(1000);
  }
  PORTD &= ~Ch_0;                                                    // выключаем все каналы

  PORTD |= Ch_1 & EEPROM.read(251);                                  // в ячейке 251 - 254 храним первоначальное состояние каждого канала
  PORTD |= Ch_2 & EEPROM.read(252);
  PORTD |= Ch_3 & EEPROM.read(253);
  // PORTD |= Ch_4 & EEPROM.read(254);



}
//------------------------------------------------------------------------------------------------
void loop() {

  if (mySwitch.available()) {                                 // если с пульта принята какая-либо команда
    byte Button = Code_Decode();                              // расшифровываем её и получаем код нажатой кнопки
    if (Button == 0xC3) {                                     // если нажата комбинация С3 (HEX), то
      byte cur_Ch = PIND & 0x78;                              // считываем, какой канал сейчас включен
      if (cur_Ch != 0) {                                      // если любой канал включен, то
        count_time = 1;                                       // деактивируется флаг таймера автовыключения до перезагрузки МК
        PORTD ^= cur_Ch;                                      // подтверждающее мигание текущими каналами
        delay(200);
        PORTD |= cur_Ch;                                      // возвращаем обратно работавший канал
        delay(300);
        PORTD ^= cur_Ch;                                      // подтверждающее мигание текущими каналами
        delay(200);
        PORTD |= cur_Ch;                                      // возвращаем обратно работавший канал
      }
    }
    if (Button == prg_mode) Programm_mode();                  // если нажата комбинация 0xFF (HEX) (все кнопки сразу) переходим в режим программирования доп.возможностей.

    for (byte i = 0 ; i < 5; i++) {                           // начинаем проверять принятую команду на соответствие сопоставленному каналу управления (ячейки с 8 по 18 резерв)
      if (Button == EEPROM.read(3 + i + pad_number * 16)) {   // перебираем все соответствия для текущего пульта. Может быть одна команда для нескольких каналов
        Ch_control (i);                                       // если любая комбинация каналов включена, то все выключить.
        old_time = millis();                                  // фиксируем время, когда был включен или выключен канал
      }
    }
    delay(300);                                               // задержка между повторными нажатиями кнопки пульта
    mySwitch.resetAvailable();                                // сброс буфера приёмника
  }
  if (((PIND & Ch_0) != 0) && (millis() - old_time > 1800000) && (count_time == 0)) {
    // если какой-либо из каналов включен, и функция автовыключения не деактивирована, то через 30 минут все каналы отключатся.
    PORTD &= ~Ch_0;                                           // выключаем все каналы сразу. При нажатии любой кнопки таймер 30 минут начинает отсчёт снова.
  }
}

