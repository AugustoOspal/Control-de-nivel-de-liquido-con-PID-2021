#include "Wire.h"
#include <Keypad.h>
#include <LiquidCrystal_I2C.h> 

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////Pines//////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ECHO_PIN 2
#define TRIGGER_PIN 3
#define PWM_OUTPUT 11
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////Configuraciones/////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ROWS 4
#define COLS 3

char hexaKeys[ROWS][COLS] = {
	{'1','2','3'},
	{'4','5','6'},
	{'7','8','9'},
	{'*','0','#',}
};

byte rowPins[ROWS] = {13, 12, 10, 9}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {8, 7, 6};

#define TECLA_BORRADO '*'
#define TECLA_INTERRUPCION '#'

#define cantidad_maxima_digitos 2
#define mensaje_input "Set point: "

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Connect via i2c, default address #0 (A0-A2 not jumpered)
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 

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

	// Imprime los datos que tenemos hasta ahora (si hay alguno)
	if (n_numeros)
	{
		for (int i = 0; i < n_numeros; i++)
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
			// Almacena el nuevo nuemero
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
			int estado = 0;
			int set_point = atoi(numeros);
			float valor_sensor_ultrasonido = leerSensorUltrasonico(3, 2);

			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("Set point : ");

			for (int i  = 0; i  < n_numeros; i++)
			{
				lcd.print(numeros[i]);
			}

			while (true)
			{
				if (set_point > valor_sensor_ultrasonido)
				{
					clearRow(1);
					lcd.setCursor(0, 1);

					valor_sensor_ultrasonido = leerSensorUltrasonico(3, 2);
					barraProgreso(set_point, valor_sensor_ultrasonido, &estado);  
				}

				else()
				{
					break;
				}
			}
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

	return pulseIn(echoPin, HIGH)  / 58 + 1;
}

int barraProgreso(float valorTecho, float valorActual, int *estado)
{
	// Estado se debe inicializar a 0

	// Digitos totales horisontales del display
	byte nDigitos = 16;
	float valorPorDigito = valorTecho / nDigitos;
	int digitosRestantes = (int) round(valorActual / valorPorDigito) - *estado;

	lcd.setCursor(0, 1);

	for (int i = 0; i < 16; i++)
	{
		lcd.print(" ");
	}

	lcd.setCursor(0, 1);
	for (int i = 0; i < digitosRestantes; i++)
	{
		lcd.print("=");
		*estado++;
	}
}

void clearRow(int row)
{
	lcd.setCursor(0, row);

	for (int i = 0; i < 16; i++)
	{
		lcd.print(" ");
	}
}
