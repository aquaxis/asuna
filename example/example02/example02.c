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
	if( i == 5000000 ){
		led[0] = ~led[0];
	}else{
		i++;
	}
}

void example02(char *led1, char *led2){
	while(1){
		example01(led1);
		example01(led2);
	}
}
