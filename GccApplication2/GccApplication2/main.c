#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

uint8_t cyfry[10] = {
	0b00111111, // 0
	0b00000110, // 1
	0b01011011, // 2
	0b01001111, // 3
	0b01100110, // 4
	0b01101101, // 5
	0b01111101, // 6
	0b00000111, // 7
	0b01111111, // 8
	0b01101111  // 9
};

int main(void)
{
	DDRD = 0xFF; // caly port D na segmenty od A do G
	DDRB = 0b00111111; // od PB0 do PB5 na pierwsze 6 diod
	DDRC = 0b00001111; // PC0, PC1 na cyfry wyswietlacza, a PC2, PC3 na diody 7, 8

	
	PORTB = 0x00;			// gasimy wszystko na start
	PORTC = 0b00000011; // PC0 i PC1 na 1 zeby wylaczyc wyswietlacz (wlaczamy go stanem niskim)

	int licznik = 1;
	int zaswiecone = 0;
	int dzies = 0;
	int jedno = 0;
	while (1)
	{
		if (zaswiecone < 8)
		{
			for (licznik = 1; licznik <= 99; licznik++)
			{
				dzies = licznik / 10;
				jedno = licznik % 10;
				for (int i = 0; i < 20; i++)		// petla odswiezajaca wyswietlacz (zeby chwile poswiecilo)
				{
														// jednosci na PC0
					PORTC |= (1 << PC0) | (1 << PC1); // wylacz obie cyfry
					PORTD = ~cyfry[jedno];         // ustaw segmenty (negacja bo wspolny plus)
					PORTC &= ~(1 << PC0);             // wlacz jednosci
					_delay_ms(2);

														// dziesiatki na PC1
					PORTC |= (1 << PC0) | (1 << PC1); // wylacz obie cyfry
					if (dzies > 0)
					{
						PORTD = ~cyfry[dzies];
						PORTC &= ~(1 << PC1);         // wlacz dziesiatki
					}
					_delay_ms(2);
				}
			}
			zaswiecone++;		// gdy 99 zapalamy kolejna diode
			if (zaswiecone == 1) PORTB |= (1 << PB0);
			if (zaswiecone == 2) PORTB |= (1 << PB1);
			if (zaswiecone == 3) PORTB |= (1 << PB2);
			if (zaswiecone == 4) PORTB |= (1 << PB3);
			if (zaswiecone == 5) PORTB |= (1 << PB4);
			if (zaswiecone == 6) PORTB |= (1 << PB5);
			if (zaswiecone == 7) PORTC |= (1 << PC2);
			if (zaswiecone == 8) PORTC |= (1 << PC3);
		}
		else
		{
			PORTC |= (1 << PC0) | (1 << PC1);  // 8 diod swieci to 99 na ekranie
			PORTD = ~cyfry[9];
			PORTC &= ~(1 << PC0);
			_delay_ms(2);

			PORTC |= (1 << PC0) | (1 << PC1);
			PORTD = ~cyfry[9];
			PORTC &= ~(1 << PC1);
			_delay_ms(2);
		}
	}
}