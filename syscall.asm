.code

NtUserGetAsyncKeyState PROC
    mov r10, rcx            ; Move the first argument to r10
    mov eax, 1044h         ; Move the syscall number for NtUserSetWindowsHookEx into eax
    syscall                 ; Perform the syscall
    ret                     ; Return from the procedure
NtUserGetAsyncKeyState ENDP

end
