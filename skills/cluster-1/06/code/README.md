# Console IO

Code communicates with HUZZAH32 through UART.
Serial communication is established and code runs in a while loop (indefinitely).

For the three modes, there is a counter which keeps track and allows code to stay in a certain block of code while that mode is applicable (until switch mode key is pressed).
Length of the input is checked, if it is a valid input for that mode, it provides its functionality. Otherwise, nothing happens.

Each of the three modes can be cycle through (indefinitely).
