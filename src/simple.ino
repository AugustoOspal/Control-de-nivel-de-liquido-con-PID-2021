#include "Wire.h"
#include <Keypad.h>
#include "Adafruit_LiquidCrystal.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////Pines//////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ECHO_PIN 10
#define TRIGGER_PIN 13
#define PWM_OUTPUT 11
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////Configuraciones/////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ROWS 4
#define COLS 4

char hexaKeys[ROWS][COLS] = {
	{'1','2','3','A'},
	{'4','5','6','B'},
	{'7','8','9','C'},
	{'*','0','#','D'}
};

byte rowPins[ROWS] = {9, 8, 7, 6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 4, 3, 2};

#define TECLA_BORRADO '*'
#define TECLA_INTERRUPCION '#'

#define cantidad_maxima_digitos 2
#define mensaje_input "Set point: "

// PID constantes
const double kp = 0;
const double ki = 0;
const double kd = 0;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Connect via i2c, default address #0 (A0-A2 not jumpered)
Adafruit_LiquidCrystal lcd(0);

// Crea un objeto a partir de la clase Keypad
Keypad keypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 


// Cantidad de numeros ingresados
int n_numeros = 0;

// Cantidad de caracteres del mensaje a mostrar. Ejm. "Set point: " tiene 11 caracteres
int mensaje_len = String(mensaje_input).length();

// Se van a almacenar todos los numeros ingresados en formato char
char numeros[cantidad_maxima_digitos];


void setup() 
{
	// Comienza comunicacion serial
	Serial.begin(9600);

	// Inicializa el display LCD
	lcd.begin(16, 2);
	lcd.setBacklight(HIGH);

	// Pin que va al inverter
	pinMode(PWM_OUTPUT, OUTPUT);
}

void loop() 
{
	lcd.setCursor(0, 0);
	lcd.print(mensaje_input);

	// Imprime los daots que tenemos hasta ahora (si hay alguno)
	if (n_numeros)
	{
		// Si llegan a querer mas de 255 digitos se tiene que cambiar a otro tipo
		for (byte i = 0; i < n_numeros; i++)
		{
			lcd.print(numeros[i]);
		}
	}

	char numero = (char) keypad.getKey();

	if (numero)
	{
		// En el caso de que este todo bien lo guardamos
		if (isDigit(numero) && n_numeros < cantidad_maxima_digitos)
		{
			numeros[n_numeros] = numero;
			
			// Lo imprimimos
			lcd.setCursor(mensaje_len + n_numeros, 0);
			lcd.print(numero);

			n_numeros++;
		}

		// Si el numero es valido pero ya no tenemos mas espacio en numeros[]
		else if (isDigit(numero) && n_numeros == cantidad_maxima_digitos)
		{
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("No hay lugar");
			delay(2000);
			lcd.clear();
		}

		// Borra el ultimo digito ingresado actualizando n_numeros asi despues lo sobrescriben
		else if (numero == TECLA_BORRADO && n_numeros > 0)
		{
			n_numeros--;

			// Va a la posicion en la que estaba el numero borrado
			lcd.setCursor(mensaje_len + n_numeros, 0);

			// Lo borra
			lcd.print(" ");

			// Vuelve a la posicion del numero borrado asi se sobrescribe
			lcd.setCursor(mensaje_len + n_numeros, 0);
		}

		// Entramos a la confirmacion
		else if (numero == TECLA_INTERRUPCION)
		{
			// Mensaje de confirmacion
			lcd.clear();
			lcd.setCursor(0,0);
			lcd.print("Seguro?");
			lcd.setCursor(0,1);
			lcd.print("No = 1  Si = 2");
			lcd.blink();

			// Hasta que no tengamos como respuesta un 1 o 2

			numero = 0;

			while (numero != 1 && numero != 2)
			{
				numero = keypad.getKey();
			}

			/* 	
				Dejamos de introducir los datos. Aca es cuando tendriamos que 
			 	procesar los datos del sensor de ultrasonido y despues mandarle 
			 	al motor las instrucciones
			*/
			if (numero == 2)
			{
				lcd.clear();
				lcd.setCursor(0, 0);
				lcd.print("Set point -> ");
				lcd.print(String(numeros));

				unsigned long tiempoActual, ultimoTiempo, diferenciaTimepo;

				int estadoBarra = 0;

				float p = 0, i = 0, d = 0;
				float setPoint, nivelActual, output, error, errorAnterior;

				ultimoTiempo = 0;
				setPoint = atoi(numeros);
				nivelActual = 0;

				// PID
				while (setPoint > nivelActual)
				{
					tiempoActual = millis();
					diferenciaTimepo = tiempoActual - ultimoTiempo;

					nivelActual = leerSensorUltrasonico(TRIGGER_PIN, ECHO_PIN);
					error = setPoint - nivelActual;

					p = error;
					i += error * diferenciaTimepo;
					d = (error - errorAnterior) / diferenciaTimepo;

					output = kp * p + ki * i + kd * d;

					if (output > 255) output = 255;
					else if (output < 0) output = 0;

					analogWrite(PWM_OUTPUT, output);

					ultimoTiempo = tiempoActual;
					errorAnterior = error;

					lcd.setCursor(0, 1);
					barraProgreso(setPoint, nivelActual, &estadoBarra);
				}
				
				lcd.setCursor(0, 1);

				for (int i = 0; i < 16; i++)
				{
					lcd.print(" ");
				}

				lcd.setCursor(0, 1);
				lcd.print("Listo");
				
				delay(200);
				lcd.clear();
			}

			// Volvemos a el menu para seguir ingresando numeros
			else
			{
				lcd.clear();
			}

			lcd.noBlink();
		}
	}
}

// Lee los datos que da el sensor de ultrasonido
float leerSensorUltrasonico (int triggerPin, int echoPin)
{
	pinMode(triggerPin, OUTPUT);

	digitalWrite(triggerPin, LOW);
	delayMicroseconds(2);
	
	// Setea el pin en HIGH por 10 micro segundos
	// (Lo especifica el manual)
	digitalWrite(triggerPin, HIGH);
	delayMicroseconds(10);
	digitalWrite(triggerPin, LOW);

	pinMode(echoPin, INPUT);
	
	return pulseIn(echoPin, HIGH);
}

int barraProgreso(float valorTecho, float valorActual, int *estado)
{
	// Estado se debe inicializar a 0

	// Digitos totales horisontales del display
	byte nDigitos = 16;
	float valorPorDigito = valorTecho / nDigitos;
	int digitosRestantes = (int) round(valorActual / valorPorDigito) - *estado;

	for (int i = 0; i < digitosRestantes; i++)
	{
		lcd.print("=");
		*estado++;
	}

}