/*
 * Copyright (C)2005-2024 AQUAXIS TECHNOLOGY.
 *  Don't remove this header.
 * When you use this source, there is a need to inherit this header.
 *
 * This software is released under the MIT License.
 * https://opensource.org/license/mit
 */

/*!
 * @note
 * LEDチカチカを連続させた場合
 */
void example01(char *led){
	int i = 0;
	int count = 0;
	while(count < 10){
		if( i == 100 ){
			i = 0;
			led[0] = ~led[0];
			count++;
		}else{
			i++;
		}
	}
}

void example02(char *led1, char *led2){
	int count = 0;
	while(count < 10){
		example01(led1);
		example01(led2);
		count++;
	}
}
