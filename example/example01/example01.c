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
 * LEDチカチカのようなプログラムです。
 */
void example01(char *led){
	int i = 0;
	while(1){
		if( i == 5000000 ){
			i = 0;
			led[0] = ~led[0];
		}else{
			i++;
		}
	}
}
