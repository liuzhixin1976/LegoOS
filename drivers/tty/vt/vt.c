/*
 * Copyright (c) 2016 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * VT102 Emulation
 */

#include <lego/tty.h>

void vt_init(void)
{
}

static int vt_putchar(struct tty_struct *tty, unsigned char ch)
{
	return 0;
}

static int vt_write(struct tty_struct *tty, const unsigned char *buf, int count)
{
	return 0;
}

static const struct tty_operations vt_ops = {
	.write	= vt_write,
	.put_char = vt_putchar,
};

struct tty_driver vt_driver = {
	.ops	= &vt_ops,
};