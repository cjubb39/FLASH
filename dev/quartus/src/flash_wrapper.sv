module flash_wrapper (
	input clk,
	input rst,

	/* HPS interface */
	output        hps_tick_irq,
	output [63:0] hps_next_process,
	input         hps_read,

	input  [63:0] hps_change_data,

	input         hps_req,
	input         hps_address /* change (1) or sched req (0) */
);

logic        sched_req;
logic        sched_grant;

logic        tick_req;
logic        tick_grant;

logic [15:0] next_process;

logic        change_req;
logic  [7:0] change_type;
logic [15:0] change_pid;
logic  [7:0] change_pri;
logic [15:0] change_state;
logic        change_grant;

/* tmps for buffering outputs to hps */
logic        hps_sched_req;
logic        hps_change_req;

logic        tick_irq;

logic [63:0] next_process_int;

/* break down incoming data */
assign change_type  = hps_change_data [7:0];
assign change_pid   = hps_change_data [23:8];
assign change_pri   = hps_change_data [31:24];
assign change_state = hps_change_data [47:32];
assign hps_next_process = next_process_int;

/* convert to sched and change requests */
assign hps_sched_req  = hps_req & !hps_address;
assign hps_change_req = hps_req & hps_address; 

assign hps_tick_irq = tick_irq;

always_ff @(posedge clk) begin
	next_process_int[15:0]  <= next_process;
	next_process_int[63:16] <= 32'b0;
end

/* interrupt handling */
always_ff @(posedge clk) begin
	if (rst) begin
	end else begin
		if (tick_req && !tick_grant) begin
			tick_grant <= 1'b1;
			tick_irq <= 1'b1;
		end else if (tick_req && tick_grant) begin
		end else if (!tick_req && tick_grant) begin
			tick_grant <= 1'b0;
		end else if (!tick_req && !tick_grant && hps_read) begin
			tick_irq <= 1'b0;
		end
	end
end

/* sched_req handling */
always_ff @(posedge clk) begin
	if (rst) begin
	end else begin
		if (hps_sched_req && !sched_req && !sched_grant) begin
			sched_req <= 1'b1;
		end else if (sched_req && !sched_grant) begin
		end else if (sched_req && sched_grant) begin
			sched_req <= 1'b0;
		end else if (!sched_req && sched_grant) begin
		end

		/*
		 * drop requests if they come in too fast
		 * the kernel won't ask for another task while its scheduling one 
		 */

	end
end


/* change_req handling */
always_ff @(posedge clk) begin
	if (rst) begin
	end else begin
		if (hps_change_req && !change_req && !change_grant) begin
			change_req <= 1'b1;
		end else if (change_req && !change_grant) begin
		end else if (change_req && change_grant) begin
			change_req <= 1'b0;
		end else if (!change_req && change_grant) begin
		end

		/*
		 * drop requests if they come in too fast
		 * the kernel won't ask for another change while its changing one 
		 */

	end
end

flash_rtl f(
	.clk(clk),
	.rst(rst),
	.sched_req(sched_req),
	.tick_grant(tick_grant),
	.change_req(change_req),
	.change_type(change_type),
	.change_pid(change_pid),
	.change_pri(change_pri),
	.change_state(change_state),
	.sched_grant(sched_grant),
	.next_process(next_process),
	.tick_req(tick_req),
	.change_grant(change_grant)
);

endmodule

