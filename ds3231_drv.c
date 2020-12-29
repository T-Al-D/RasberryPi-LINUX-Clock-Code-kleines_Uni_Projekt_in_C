/******************************************************************************
* Institut für Rechnerarchitektur und Systemprogrammierung		      *
* Universität Kassel							      *
*******************************************************************************
* Benutzeraccount : sysprog-04						      *
*******************************************************************************
* Author-1 : Noah Schager   						      *
*******************************************************************************
* Author-2 : Taha Al Dailami						      *
*******************************************************************************
* Beschreibung: Treibersoftware für RTC-Chip Uhr			      *
* Version: Finale Version 1.0 (Aufgrund von Urheberrechts zensiert! Nur Teile *
* welche ausschießlich von unserer Gruppe geschrieben wurden veröffentlicht)  *						      *
******************************************************************************/

/* Ab hier eigener Code*/
/* assembly macht aus einer u8 BCD nummer ein Integer*/
static int assembly (int number){
	int number_first_digit, number_tens;
	number_first_digit = number & 0b00001111; 
	number_tens = number >> 4;
	number = number_first_digit +(10 * number_tens);
	return number;
}

/* reverse_assembly macht aus einem Integer eine u8 Nummer*/
static int reverse_assembly (int second_number, int first_number){
	u8 BCD_number;
	BCD_number = 0b00000000;
	BCD_number = second_number;
	BCD_number = (BCD_number << 4); 
	BCD_number = BCD_number | first_number;
	return BCD_number;
}

/* Diese Funktion gibt den Monat in Form eines Strings wieder.*/
static const char* months_to_string (int num){
	switch (num)
	{
	case 1 : return "Januar";break;
	case 2 : return "Feburar";break;
	case 3 : return "Maerz";break;
	case 4 : return "April";break;
	case 5 : return "Mai";break;
	case 6 : return "Juni";break;
	case 7 : return "Juli";break;
	case 8 : return "August";break;
	case 9 : return "September";break;
	case 10 : return "Oktober";break;
	case 11 : return "November";break;
	case 12 : return "Dezember";break;
	default : return " "; break;
	}
}

/* isdigit prüft ob es sich (im char) um eine Nummer handelt!*/
static bool isdigit(char char_num){
	switch (char_num)
	{
	case '0': case '1' : case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
	return true; break;
	default : return false; break; 
	}
}

/* C-Datenstrucktur in der Daten abgespeichert werden*/
static struct status {
	bool osf_Flag, bsy_Flag;
	int temperature;
}stat;

/* Bei jedem Zugriff (echo & cat) wird mein_open ausgeführt*/
static int mein_open(struct inode *inode, struct file *file){

	u8 status_register, temp_register_upp_byte, temp_register_low_byte;
	u8 w_osf_Flag;
	int write_finish; 
	
	status_register  =  i2c_smbus_read_byte_data(ds3231_client, 0x0F);
	temp_register_upp_byte  =  i2c_smbus_read_byte_data(ds3231_client, 0x11);
	temp_register_low_byte  =  i2c_smbus_read_byte_data(ds3231_client, 0x12);
	
	stat.osf_Flag = (status_register & 0b10000000);
	stat.bsy_Flag = (status_register & 0b00000100);
	stat.temperature = (temp_register_upp_byte) ;
	
	/* Aufrunden mit der Zahl hinter Komma! 7. Bit gibt 0,5 an!*/
	if (temp_register_low_byte & 0b10000000){
		stat.temperature += 1;
	}
	
	/* BSY_Flag muss 0 sein um Tempratur sinnvoll zu erfragen!, sonst sind die TCXO -Funktion noch aktiv!*/
	if (stat.bsy_Flag == false){
		if(stat.temperature <= (-40)){
		printk("%03i °C!, Temprature too low! \n ",stat.temperature);
		}else if(stat.temperature >= 85){
		printk("%03i °C!, Temprature too hot! \n",stat.temperature);
		}else{
		printk("%03i °C, Everything OK! \n",stat.temperature);
		}
	}
	
	/* Wenn osf_Flag == true ist bedeutet das, dass der Oscillator gestoppt wurde!*/
	if (stat.osf_Flag == true){
		w_osf_Flag = status_register & (~0b10000000);
		write_finish = i2c_smbus_write_byte_data(ds3231_client, 0x0F, w_osf_Flag);
			if (write_finish == 0){
			printk("FEHLER BEIM SCHREIBEN (osf_Flag)!");}
		return -EAGAIN ;
	}
	return 0;  
}

static int mein_close(struct inode *inode, struct file *file){
	return 0;
}

/* auslesen aus den RTC-Chip*/
static ssize_t mein_read(struct file *file, char __user *puffer, size_t bytes, loff_t *offset){
	
	/*printed wird benötigt um die Endlosschleife bei Ausgabe zu untedrücken*/
  	static bool printed = false;               
	char final_string[31];  
	int count, N;
	N = 31;
	
	unsigned int century;
	u8 years, months, days, hours, minutes, seconds;

	months  = i2c_smbus_read_byte_data(ds3231_client, 0x05);
	century = (( months & 0b10000000) >> 7);
	months = months & 0b00011111;
	months = assembly( months);
	
	years  =  i2c_smbus_read_byte_data(ds3231_client, 0x06);
	years = assembly(years);
	
	days  = i2c_smbus_read_byte_data(ds3231_client, 0x04);
	days = days & 0b00111111;
	days = assembly(days);
		
	hours = i2c_smbus_read_byte_data(ds3231_client, 0x02);
	if((hours >> 6 ) == 0){
 		hours = hours & 0b00111111;
		hours = assembly(hours);
	}else{
		
 		hours = hours & 0b00111111;
		if(( hours >> 5) == 0){
 			hours = hours & 0b00011111;
			hours = assembly(hours);
		}else{
			hours = hours & 0b00011111;
			hours = assembly(hours)+12;
		}	
	}
	
	minutes = i2c_smbus_read_byte_data(ds3231_client, 0x01);
	minutes = assembly(minutes);
	
	seconds = i2c_smbus_read_byte_data(ds3231_client, 0x00);
	seconds = assembly(seconds);
	
	/* Hier wird Ausgabe-String zusammengefügt*/
	snprintf(final_string, N, "%02i. %s %02i:%02i:%02i 2%01i%02i \n", days, months_to_string(months), hours, minutes, seconds, century, years);	 	 
	
	if(printed == false){
		count = copy_to_user(puffer, final_string, N);
		printed = true;
		return N -count;
	}else if (printed == true){
		printed  = false;
		return 0;
	}
	return 0;
}

/* Schreibt in den RTC-CHIP oder C-Datenstrucktur*/
static ssize_t mein_write(struct file *file, const char __user* puffer, size_t bytes, loff_t *offset){
	
	char input_string[bytes];
	int missing_bytes;
	missing_bytes = copy_from_user(input_string, puffer, bytes);
	
	if  (input_string[0] == '?'){

		char sign;
		int tempe_second_digit, tempe_first_digit;

		sign  =  input_string[1];	
		tempe_second_digit = input_string[2]- '0';
		tempe_first_digit = input_string[3]- '0';
			
		if(sign == '-'){
			stat.temperature = (-1)*((tempe_second_digit * 10)+ tempe_first_digit);
			if (stat.temperature <= -40){
				printk( "The temperature you choose is too cold!");
			}
		}else{
			stat.temperature = ((sign -'0') * 100) + (tempe_second_digit * 10) + tempe_first_digit;
			if (stat.temperature >= 85){
				printk( "The temperatur you choose is too hot!");
			}
		}
		printk("%03i °C, wurde eingetragen! \n",stat.temperature);
	}else{
	
	int i;
	int len_string, calendar_year, write_finish;
	int year_forth_digit, year_third_digit, year_second_digit, year_first_digit;
	int month_second_digit, month_first_digit;
	int day_second_digit, day_first_digit;
	int hour_second_digit, hour_first_digit;
	int minute_second_digit, minute_first_digit;
	int seconds_second_digit, seconds_first_digit;
	
	char hyphen_1, hyphen_2;
	char colon_1, colon_2;

	unsigned int w_century;
	u8 w_years, w_months, w_days, w_hours, w_minutes, w_seconds;
	
	len_string = sizeof(input_string)/sizeof(char);
	
	/*Beispiel:2019-04-15 19:45:23  (19*char)*/
	year_forth_digit  = input_string[0]-'0';
	year_third_digit  = input_string[1]-'0';
	year_second_digit  = input_string[2]-'0';
	year_first_digit  = input_string[3]-'0';
	hyphen_1	 =  input_string[4];
	month_second_digit  = input_string[5]-'0';
	month_first_digit  = input_string[6]-'0';
	hyphen_2	=  input_string[7];
	day_second_digit  = input_string[8]-'0';
	day_first_digit  = input_string[9]-'0';

	hour_second_digit  = input_string[11]-'0';
	hour_first_digit  = input_string[12]-'0';
	colon_1		 = input_string[13];
	minute_second_digit  = input_string[14]-'0';
	minute_first_digit  = input_string[15]-'0';
	colon_2		  = input_string[16];
	seconds_second_digit  = input_string[17]-'0';
	seconds_first_digit  = input_string[18]-'0';
	
	calendar_year = year_first_digit + (year_second_digit*10) + (year_third_digit*100) + (year_forth_digit*1000);	
	w_century = (year_third_digit) << 7;
	
	/*for Schleife prüft ob wirklich nur Zahlen mitgegeben wurden*/
	for(i =0; i<19;i++){
		if(i != 4 && i != 7 && i != 10 && i != 13 && i != 16 ){
			if(!(isdigit(input_string[i]))){
				printk("No Digit at position %02i" ,i);
				return -EINVAL;
			}	
		}
	}
	/* Prüfen ob Zahlen an den Zehner sind und kein Minus!*/	
	//if ((!(isdigit(input_string[11])) || !(isdigit(input_string[14])) || !(isdigit(input_string[17])))){
	//	printk(" FEHLER 0!");
	//	return -EINVAL ;}	
	/* Prüfe auf Laenge des Arrays*/
	if(len_string != 20){
		printk("FEHLER 1");
		return -EINVAL;}
	/* Prüft auf Doppelpunkte!*/
	if(colon_1 != ':' || colon_2 != ':'){
		printk("FEHLER 2");
		return -EINVAL;}
	/*Bindestriche müssen stimmen!*/
	if(hyphen_1 != '-' || hyphen_2 != '-'){
		printk("FEHLER 3");
		return -EINVAL;}
	/*Nur 2000 ist akzeptabel */
	if(year_forth_digit != 2){
		printk("FEHLER  4 ");
		return -EOVERFLOW;}
	/*Nur bis 1 im Hunderter*/
	if(year_third_digit > 1){
		printk("FEHLER  5 ");
		return -EOVERFLOW;}
	/*Schaltjahr von 2000-2199 !!! Februar darf 29 Tage haben, sonst nur 28 */
	if(month_second_digit == 0 && month_first_digit == 2){
		if(day_second_digit > 2){ 
			printk("FEHLER  6 ");
			return -EINVAL;}
		if(!((calendar_year % 4 == 0 && calendar_year % 100 != 0) || calendar_year == 2000)){	
			if (day_first_digit > 8 && day_second_digit == 2){
				printk("FEHLER  7 ");
				return -EINVAL;}}}
	/* Monat zehner Zahl darf nicht mehr als 1 sein*/
	if(month_second_digit > 1){
		printk("FEHLER  8");
		return -EINVAL;}
	/* Monat einer darf nicht mehr als 2 sein, falls monat zehner 1 ist */
	if(month_second_digit == 1 && month_first_digit > 2){
		printk("FEHLER  9 ");
		return -EINVAL;}
	/* Tag Zehner nicht über 3 */
	if(day_second_digit > 3){
		printk("FEHLER  10 ");
		return -EINVAL;}
	/* Tag Einer bei 3 (Zehner) darf nicht mehr als 1 sein (Jan., Maerz, Mai, Juli, August) */
	if((month_second_digit == 0) && (month_first_digit ==  1 || month_first_digit == 3 || month_first_digit == 5 || month_first_digit == 7 || month_first_digit == 8 )){ 
		if(day_second_digit == 3){
			if (day_first_digit > 1){
				printk("FEHLER  11");
				return -EINVAL;}}}
	/* Tag Einer bei 3 (Zehner) darf nicht mehr als 0 sein (April, Juni, September) */
	if((month_second_digit == 0) && (month_first_digit ==  4 || month_first_digit == 6 || month_first_digit == 9 )){ 
		if(day_second_digit == 3){
			if (day_first_digit > 0){
				printk("FEHLER 12 ");
				return -EINVAL;}}}
	/* Tag Einer bei 3 (Zehner) darf nicht mehr als 0 sein (November) */
	if((month_second_digit == 1) && (month_first_digit ==  1 )){ 
		if(day_second_digit == 3){
			if (day_first_digit > 0){
				printk("FEHLER  13");
				return -EINVAL;}}}
	/* Tag Einer bei 3 (Zehner) darf nicht mehr als 1 sein (Okt., Dez.) */
	if((month_second_digit == 1) && (month_first_digit ==  0 || month_first_digit == 2 )){ 
		if(day_second_digit == 3){
			if (day_first_digit > 1){
		 		printk("FEHLER  14 ");
				return -EINVAL;}}}
	/* Stunden zehner nicht mehr als 2 */
	if(hour_second_digit > 2 && isdigit(hour_second_digit)){
		printk("FEHLER  15 ");
		return -EINVAL;}
	/* Stunden einer darf nicht mehr als 3 sein, stundenzeiger 2 ist  */
	if(hour_second_digit == 2 && hour_first_digit > 3){
		printk("FEHLER 16 ");
		return -EINVAL;}
	/* Minute oder Sekunde Zehner nicht mehr als 5 */
	if(minute_second_digit > 5 || seconds_second_digit > 5){
		printk("FEHLER  17 ");
		return -EINVAL;}
	
	/* Variablen setzen!*/
	w_seconds = reverse_assembly(seconds_second_digit, seconds_first_digit);
	w_minutes = reverse_assembly(minute_second_digit, minute_first_digit);
	w_hours = reverse_assembly(hour_second_digit, hour_first_digit);
	w_hours = w_hours & 0b00111111;
	w_days = reverse_assembly(day_second_digit, day_first_digit);
	w_months = reverse_assembly(month_second_digit, month_first_digit);
	w_months = w_months | w_century;
	w_years = reverse_assembly(year_second_digit, year_first_digit);
	
	write_finish = i2c_smbus_write_byte_data(ds3231_client, 0x00, w_seconds);
	if(write_finish != 0){
		printk("FEHLER BEIM SCHREIBEN (seconds)!");}
	write_finish = i2c_smbus_write_byte_data(ds3231_client, 0x01, w_minutes);
	if(write_finish != 0){
		printk("FEHLER BEIM SCHREIBEN (minutes)!");}
	write_finish = i2c_smbus_write_byte_data(ds3231_client, 0x02, w_hours);
	if(write_finish != 0){
		printk("FEHLER BEIM SCHREIBEN (hours)!");}
	write_finish = i2c_smbus_write_byte_data(ds3231_client, 0x04, w_days);
	if(write_finish != 0){
		printk("FEHLER BEIM SCHREIBEN (days)!");}
	write_finish =i2c_smbus_write_byte_data(ds3231_client, 0x05, w_months);
	if(write_finish != 0){
		printk("FEHLER BEIM SCHREIBEN (months)!");}
	write_finish =i2c_smbus_write_byte_data(ds3231_client, 0x06, w_years);
	if(write_finish != 0){
		printk("FEHLER BEIM SCHREIBEN (years)!");}
	}
	return bytes;
}

/*Zusätzlich bedanken wir uns bei STEFAN SCHMELZ für seine Hilfe, falls wir nicht weiter wussten. Danke!*/
