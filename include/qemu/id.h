/*
 * id.h
 *
 *  Created on: Aug 21, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_ID_H_
#define QEMU_ID_H_

typedef enum IdSubSystem {
	ID_QDEV,
	ID_BLOCK,
	ID_MAX
} IdSubSystems;

char *id_generate(IdSubSystems id);
bool id_wellformed(const char *id);

#endif /* QEMU_ID_H_ */
