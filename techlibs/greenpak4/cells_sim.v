module GP_DFF(input D, CLK, output reg Q);
	parameter [0:0] INIT = 1'bx;
	initial Q = INIT;
	always @(posedge CLK) begin
		Q <= D;
	end
endmodule

module GP_DFFS(input D, CLK, nSET, output reg Q);
	parameter [0:0] INIT = 1'bx;
	initial Q = INIT;
	always @(posedge CLK, negedge nSET) begin
		if (!nSET)
			Q <= 1'b1;
		else
			Q <= D;
	end
endmodule

module GP_DFFR(input D, CLK, nRST, output reg Q);
	parameter [0:0] INIT = 1'bx;
	initial Q = INIT;
	always @(posedge CLK, negedge nRST) begin
		if (!nRST)
			Q <= 1'b0;
		else
			Q <= D;
	end
endmodule

module GP_DFFSR(input D, CLK, nSR, output reg Q);
	parameter [0:0] INIT = 1'bx;
	parameter [0:0] SRMODE = 1'bx;
	initial Q = INIT;
	always @(posedge CLK, negedge nSR) begin
		if (!nSR)
			Q <= SRMODE;
		else
			Q <= D;
	end
endmodule

module GP_INV(input IN, output OUT);
	assign OUT = ~IN;
endmodule

module GP_2LUT(input IN0, IN1, output OUT);
	parameter [3:0] INIT = 0;
	assign OUT = INIT[{IN1, IN0}];
endmodule

module GP_3LUT(input IN0, IN1, IN2, output OUT);
	parameter [7:0] INIT = 0;
	assign OUT = INIT[{IN2, IN1, IN0}];
endmodule

module GP_4LUT(input IN0, IN1, IN2, IN3, output OUT);
	parameter [15:0] INIT = 0;
	assign OUT = INIT[{IN3, IN2, IN1, IN0}];
endmodule

module GP_VDD(output OUT);
       assign OUT = 1;
endmodule

module GP_VSS(output OUT);
       assign OUT = 0;
endmodule

module GP_LFOSC(input PWRDN, output reg CLKOUT);
	
	parameter PWRDN_EN = 0;
	parameter AUTO_PWRDN = 0;
	parameter OUT_DIV = 1;
	
	initial CLKOUT = 0;
	
	always begin
		if(PWRDN)
			clkout = 0;
		else begin
			//half period of 1730 Hz
			#289017;
			clkout = ~clkout;
		end
	end
	
endmodule

module GP_COUNT8(input CLK, input wire RST, output reg OUT);

	parameter RESET_MODE 	= "RISING";	
	
	parameter COUNT_TO		= 8'h1;
	parameter CLKIN_DIVIDE	= 1;
	
	//more complex hard IP blocks are not supported for simulation yet
	
	reg[7:0] count = COUNT_TO;
	
	//Combinatorially output whenever we wrap low
	always @(*) begin
		OUT <= (count == 8'h0);
	end
	
	//POR or SYSRST reset value is COUNT_TO. Datasheet is unclear but conversations w/ Silego confirm.
	//Runtime reset value is clearly 0 except in count/FSM cells where it's configurable but we leave at 0 for now.
	//Datasheet seems to indicate that reset is asynchronous, but for now we model as sync due to Yosys issues...
	always @(posedge CLK) begin
		
		count		<= count - 1'd1;
		
		if(count == 0)
			count	<= COUNT_MAX;
			
		/*
		if((RESET_MODE == "RISING") && RST)
			count	<= 0;
		if((RESET_MODE == "FALLING") && !RST)
			count	<= 0;
		if((RESET_MODE == "BOTH") && RST)
			count	<= 0;
		*/			
	end

endmodule

module GP_COUNT14(input CLK, input wire RST, output reg OUT);

	parameter RESET_MODE 	= "RISING";	
	
	parameter COUNT_TO		= 14'h1;
	parameter CLKIN_DIVIDE	= 1;
	
	//more complex hard IP blocks are not supported for simulation yet

endmodule

//keep constraint needed to prevent optimization since we have no outputs
(* keep *)
module GP_SYSRESET(input RST);
	parameter RESET_MODE = "RISING";
	
	//cannot simulate whole system reset
	
endmodule
