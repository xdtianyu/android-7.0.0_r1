	.text
	.set	noat
	.set	noreorder
	.set	nomacro
test_mxu:

.macro test1	insn
	\insn	xr1, xr2, xr3, xr4,AA,WW
	\insn	xr1, xr2, xr3, xr4,AA,LW
	\insn	xr1, xr2, xr3, xr4,AA,HW
	\insn	xr1, xr2, xr3, xr4,AA,XW

	\insn	xr1, xr2, xr3, xr4,AA,0
	\insn	xr1, xr2, xr3, xr4,AA,1
	\insn	xr1, xr2, xr3, xr4,AA,2
	\insn	xr1, xr2, xr3, xr4,AA,3

	\insn	xr1, xr2, xr3, xr4,AS,WW
	\insn	xr1, xr2, xr3, xr4,AS,LW
	\insn	xr1, xr2, xr3, xr4,AS,HW
	\insn	xr1, xr2, xr3, xr4,AS,XW

	\insn	xr1, xr2, xr3, xr4,AS,0
	\insn	xr1, xr2, xr3, xr4,AS,1
	\insn	xr1, xr2, xr3, xr4,AS,2
	\insn	xr1, xr2, xr3, xr4,AS,3

	\insn	xr1, xr2, xr3, xr4,SA,WW
	\insn	xr1, xr2, xr3, xr4,SA,LW
	\insn	xr1, xr2, xr3, xr4,SA,HW
	\insn	xr1, xr2, xr3, xr4,SA,XW

	\insn	xr1, xr2, xr3, xr4,SA,0
	\insn	xr1, xr2, xr3, xr4,SA,1
	\insn	xr1, xr2, xr3, xr4,SA,2
	\insn	xr1, xr2, xr3, xr4,SA,3

	\insn	xr1, xr2, xr3, xr4,SS,WW
	\insn	xr1, xr2, xr3, xr4,SS,LW
	\insn	xr1, xr2, xr3, xr4,SS,HW
	\insn	xr1, xr2, xr3, xr4,SS,XW

	\insn	xr1, xr2, xr3, xr4,SS,0
	\insn	xr1, xr2, xr3, xr4,SS,1
	\insn	xr1, xr2, xr3, xr4,SS,2
	\insn	xr1, xr2, xr3, xr4,SS,3
.endm

.macro test2	insn
	\insn	xr1, xr2, xr3, xr4, AA
	\insn	xr1, xr2, xr3, xr4, SA
	\insn	xr1, xr2, xr3, xr4, AS
	\insn	xr1, xr2, xr3, xr4, SS
.endm

.macro test3 insn
	\insn xr1, $2,510, ptn0
	\insn xr1, $2,510, ptn1

	\insn xr1, $2,-512, ptn0
	\insn xr1, $2,-512, ptn1
.endm

.macro test4 insn
	\insn xr1, $2,510, ptn0
	\insn xr1, $2,510, ptn1
	\insn xr1, $2,510, ptn2
	\insn xr1, $2,510, ptn3

	\insn xr1, $2,-512, ptn0
	\insn xr1, $2,-512, ptn1
	\insn xr1, $2,-512, ptn2
	\insn xr1, $2,-512, ptn3
.endm

.macro test5 insn
	\insn xr1, $2,127, ptn0
	\insn xr1, $2,127, ptn1
	\insn xr1, $2,127, ptn2
	\insn xr1, $2,127, ptn3
	\insn xr1, $2,127, ptn4
	\insn xr1, $2,127, ptn5
	\insn xr1, $2,127, ptn6
	\insn xr1, $2,127, ptn7

	\insn xr1, $2,-128, ptn0
	\insn xr1, $2,-128, ptn1
	\insn xr1, $2,-128, ptn2
	\insn xr1, $2,-128, ptn3
	\insn xr1, $2,-128, ptn4
	\insn xr1, $2,-128, ptn5
	\insn xr1, $2,-128, ptn6
	\insn xr1, $2,-128, ptn7
.endm

.macro test6 insn
	\insn xr1, $2,127, ptn0
	\insn xr1, $2,127, ptn1
	\insn xr1, $2,127, ptn2
	\insn xr1, $2,127, ptn3

	\insn xr1, $2,-128, ptn0
	\insn xr1, $2,-128, ptn1
	\insn xr1, $2,-128, ptn2
	\insn xr1, $2,-128, ptn3
.endm
	mfc1	$2, $2
	mfc1	$2, $f1
	mtc1	$2, $2
	mtc1	$2, $f1
	mfhc1	$2, $2
	mfhc1	$2, $f2
	mthc1	$2, $2
	mthc1	$2, $f2
	test1	d16mac
	test1	d16macf
	test1	d16madl
	test1	q16add
	test1	d16mace

	d16mul	xr1, xr2, xr3, xr4,WW
	d16mul	xr1, xr2, xr3, xr4,LW
	d16mul	xr1, xr2, xr3, xr4,HW
	d16mul	xr1, xr2, xr3, xr4,XW

	d16mul	xr1, xr2, xr3, xr4,0
	d16mul	xr1, xr2, xr3, xr4,1
	d16mul	xr1, xr2, xr3, xr4,2
	d16mul	xr1, xr2, xr3, xr4,3

	d16mulf	xr1, xr2, xr3, WW
	d16mulf	xr1, xr2, xr3, LW
	d16mulf	xr1, xr2, xr3, HW
	d16mulf	xr1, xr2, xr3, XW

	d16mulf	xr1, xr2, xr3, 0
	d16mulf	xr1, xr2, xr3, 1
	d16mulf	xr1, xr2, xr3, 2
	d16mulf	xr1, xr2, xr3, 3

	d16mule	xr1, xr2, xr3, xr4, WW
	d16mule	xr1, xr2, xr3, xr4, LW
	d16mule	xr1, xr2, xr3, xr4, HW
	d16mule	xr1, xr2, xr3, xr4, XW

	d16mule	xr1, xr2, xr3, xr4, 0
	d16mule	xr1, xr2, xr3, xr4, 1
	d16mule	xr1, xr2, xr3, xr4, 2
	d16mule	xr1, xr2, xr3, xr4, 3

	s16mad	xr1, xr2, xr3, xr4,A,WW
	s16mad	xr1, xr2, xr3, xr4,A,LW
	s16mad	xr1, xr2, xr3, xr4,A,HW
	s16mad	xr1, xr2, xr3, xr4,A,XW

	s16mad	xr1, xr2, xr3, xr4,A,0
	s16mad	xr1, xr2, xr3, xr4,A,1
	s16mad	xr1, xr2, xr3, xr4,A,2
	s16mad	xr1, xr2, xr3, xr4,A,3

	s16mad	xr1, xr2, xr3, xr4,S,WW
	s16mad	xr1, xr2, xr3, xr4,S,LW
	s16mad	xr1, xr2, xr3, xr4,S,HW
	s16mad	xr1, xr2, xr3, xr4,S,XW

	s16mad	xr1, xr2, xr3, xr4,S,0
	s16mad	xr1, xr2, xr3, xr4,S,1
	s16mad	xr1, xr2, xr3, xr4,S,2
	s16mad	xr1, xr2, xr3, xr4,S,3

	q8mul	xr1, xr2, xr3, xr4
	q8mulsu	xr1, xr2, xr3, xr4
	q8movz	xr1, xr2, xr3
	q8movn	xr1, xr2, xr3
	d16movz xr1, xr2, xr3
	d16movn xr1, xr2, xr3
	s32movz xr1, xr2, xr3
	s32movn xr1, xr2, xr3

	test2	q8mac
	test2	q8macsu

	q16scop xr1, xr2, xr3, xr4

	test2	q8madl

	s32sfl	xr1, xr2, xr3, xr4, ptn0
	s32sfl	xr1, xr2, xr3, xr4, ptn1
	s32sfl	xr1, xr2, xr3, xr4, ptn2
	s32sfl	xr1, xr2, xr3, xr4, ptn3

	q8sad	xr1, xr2, xr3, xr4

	test2	d32add

	d32addc xr1, xr2, xr3, xr4

	test2	d32acc
	test2	d32accm
	test2	d32asum
	test2	q16acc
	test2	q16accm
	test2	d16asum
	test2	q8adde

	d8sum	xr1, xr2, xr3
	d8sumc	xr1, xr2, xr3
	test2	q8acce

	s32cps	xr1, xr2, xr3
	d16cps	xr1, xr2, xr3
	q8abd	xr1, xr2, xr3
	q16sat	xr1, xr2, xr3

	s32slt	xr1, xr2, xr3
	d16slt	xr1, xr2, xr3
	d16avg	xr1, xr2, xr3
	d16avgr	xr1, xr2, xr3
	q8avg	xr1, xr2, xr3
	q8avgr	xr1, xr2, xr3
	q8add	xr1, xr2, xr3,AA
	q8add	xr1, xr2, xr3,AS
	q8add	xr1, xr2, xr3,SA
	q8add	xr1, xr2, xr3,SS

	s32max	xr1, xr2, xr3
	s32min	xr1, xr2, xr3
	d16max	xr1, xr2, xr3
	d16min	xr1, xr2, xr3
	q8max	xr1, xr2, xr3
	q8min	xr1, xr2, xr3
	q8slt	xr1, xr2, xr3
	q8sltu	xr1, xr2, xr3

	d32sll xr0, xr2, xr3, xr4, 15
	d32slr xr1, xr2, xr3, xr4, 15
	d32sarl xr1, xr2, xr3, 15
	d32sar xr1, xr2, xr3, xr4, 15
	q16sll xr1, xr2, xr3, xr4, 15
	q16slr xr1, xr2, xr3, xr4, 15
	q16sar xr1, xr2, xr3, xr4, 15

	d32sllv xr1, xr2, $0
	d32slrv xr1, xr2, $0
	d32sarv xr1, xr2, $0
	q16sllv xr1, xr2, $0
	q16slrv xr1, xr2, $0
	q16sarv xr1, xr2, $0

	s32madd xr1, xr2, $0, $2
	s32maddu xr1, xr2, $0, $2
	s32msub xr1, xr2, $0, $2
	s32msubu xr1, xr2, $0, $2
	s32mul xr1, xr2, $0, $2
	s32mulu xr1, xr2, $0, $2
	s32extrv xr1, xr2, $0, $2
	s32extr	xr1, xr2, $0, 31

	d32sarw	xr1, xr2, xr3, $0
	s32aln xr1, xr2, xr3, $0
	s32alni	xr1, xr2, xr3, ptn0
	s32alni	xr1, xr2, xr3, ptn1
	s32alni	xr1, xr2, xr3, ptn2
	s32alni	xr1, xr2, xr3, ptn3
	s32alni	xr1, xr2, xr3, ptn4
	s32lui xr1, 127, ptn0
	s32lui xr1, 127, ptn1
	s32lui xr1, 127, ptn2
	s32lui xr1, 127, ptn3
	s32lui xr1, 127, ptn4
	s32lui xr1, 127, ptn5
	s32lui xr1, 127, ptn6
	s32lui xr1, 127, ptn7
	s32lui xr1, -128, ptn0
	s32lui xr1, -128, ptn1
	s32lui xr1, -128, ptn2
	s32lui xr1, -128, ptn3
	s32lui xr1, -128, ptn4
	s32lui xr1, -128, ptn5
	s32lui xr1, -128, ptn6
	s32lui xr1, -128, ptn7
	s32lui xr1, 255, ptn0
	s32lui xr1, 255, ptn1
	s32lui xr1, 255, ptn2
	s32lui xr1, 255, ptn3
	s32lui xr1, 255, ptn4
	s32lui xr1, 255, ptn5
	s32lui xr1, 255, ptn6
	s32lui xr1, 255, ptn7
	s32nor	xr1, xr2, xr3
	s32and	xr1, xr2, xr3
	s32or	xr1, xr2, xr3
	s32xor	xr1, xr2, xr3

	lxb	$0, $2, $4, 2
	lxbu	$0, $2, $4, 2
	lxh	$0, $2, $4, 2
	lxhu	$0, $2, $4, 2
	lxw	$0, $2, $4, 2

	test3	s16std
	test3	s16sdi
	test4	s16ldd
	test4	s16ldi

	s32m2i	xr1, $4
	s32i2m	xr1, $4

	s32lddv	xr1, $0, $2, 2
	s32lddvr	xr1, $0, $2, 2
	s32stdv	xr1, $0, $2, 2
	s32stdvr	xr1, $0, $2, 2
	s32ldiv	xr1, $0, $2, 2
	s32ldivr	xr1, $0, $2, 2
	s32sdiv	xr1, $0, $2, 2
	s32sdivr	xr1, $0, $2, 2
	s32ldd	xr1, $0, 2044
	s32ldd	xr1, $0, -2048
	s32lddr	xr1, $0, 2044
	s32lddr	xr1, $0, -2048
	s32std	xr1, $0, 2044
	s32std	xr1, $0, -2048
	s32stdr	xr1, $0, 2044
	s32stdr	xr1, $0, -2048
	s32ldi	xr1, $0, 2044
	s32ldi	xr1, $0, -2048
	s32ldir	xr1, $0, 2044
	s32ldir	xr1, $0, -2048
	s32sdi	xr1, $0, 2044
	s32sdi	xr1, $0, -2048
	s32sdir	xr1, $0, 2044
	s32sdir	xr1, $0, -2048

	test5	s8ldd
	test5	s8ldi
	test6	s8std
	test6	s8sdi

# Force at least 8 (non-delay-slot) zero bytes, to make 'objdump' print ...
	.space  8
