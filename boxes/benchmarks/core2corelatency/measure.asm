
; --------------------------------------------------------------------------------
section .text
; --------------------------------------------------------------------------------

; ----------------------------------------
; uint64_t _thread_write_main(uint64_t cycle, uint64_t* read_ready, uint64_t* write_ready, uint64_T* data);
; MSVC: x64, __cdecl
; GCC: ?????????
; ----------------------------------------
global _thread_write_main
_thread_write_main:
	mov				rax, rbp
	mov				rbp, rsp
; Arguments
	; [rbp-8] data
	push			r9			; data
	; [rbp-16] write_ready
	push			r8			; write_ready
	; [rbp-24] read_ready
	push			rdx			; read_ready
	; [rbp-32] cycle
	push			rcx			; cycle
	; Store State
	push			rbx
	push			rax
;	int 3
; Signal we are ready to write.
	mov				rax, qword [rbp-32] ; cycle
	mov				rbx, qword [rbp-16] ; write_ready
	lock xchg		qword [rbx], rax
; Wait until read task is ready again.
	mov				rbx, qword [rbp-24] ; read_ready
.wait_read_ready:
	mov				rax, qword [rbp-32] ; cycle
	lock cmpxchg	qword [rbx], rax
	jne	.wait_read_ready
; Capture time and flag done.
	rdtsc ; EAX and EBX contain the data.
	shl				qword rdx, 32
	add				rax, rdx
	mov				rcx, qword [rbp-8]
	lock xchg		qword [rcx], rax
; Wait until other side resets data.
	mov				rbx, qword [rbp-8]
.wait_read_done:
	mov				rax, 0
	lock cmpxchg	qword [rbx], rax
	jne .wait_read_done
; Restore State
	pop				rbp
	pop				rbx
	pop				rcx
	pop				rdx
	pop				r8
	pop				r9
	ret 
; End-of-Function

; ----------------------------------------
; uint64_t _thread_read_main(uint64_t cycle, uint64_t* read_ready, uint64_t* write_ready, uint64_T* data);
; MSVC: x64, __cdecl
; GCC: ?????????
; ----------------------------------------
global _thread_read_main
_thread_read_main:
	mov				rax, rbp
	mov				rbp, rsp
; Arguments
	; [rbp-8] data
	push			r9			; data
	; [rbp-16] write_ready
	push			r8			; write_ready
	; [rbp-24] read_ready
	push			rdx			; read_ready
	; [rbp-32] cycle
	push			rcx			; cycle
	; Store State
	push			rbx
	push			rax
;	int 3
; Wait until write task is ready.
	mov				rbx, qword [rbp-16] ; write_ready
.wait_write_ready:
	mov				rax, qword [rbp-32] ; cycle
	lock cmpxchg	qword [rbx], rax
	jne	.wait_write_ready
; Signal we are ready to read.
	mov				rax, qword [rbp-32] ; cycle
	mov				rbx, qword [rbp-24] ; write_ready
	lock xchg		qword [rbx], rax
; Wait until write thread writes data
	mov				rbx, qword [rbp-8]
.wait_read_done:
	mov				rax, 0
	lock cmpxchg	qword [rbx], rax
	je .wait_read_done
; Capture time and return.
	rdtsc ; EAX and EBX contain the data.
	shl				qword rdx, 32
	add				rax, rdx
; Restore State
	pop				rbp
	pop				rbx
	pop				rcx
	pop				rdx
	pop				r8
	pop				r9
	ret 
; End-of-Function
