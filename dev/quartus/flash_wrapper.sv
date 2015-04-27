module flash_wrapper (
	input clk,
	input rst,

	/* flash interface */
	output        f_sched_req,
	input         f_sched_grant,

	input         f_tick_req,
	output        f_tick_grant,

	input  [15:0] f_next_process,

	output        f_change_req,
	output  [7:0] f_change_type,
	output [15:0] f_change_pid,
	output  [7:0] f_change_pri,
	output [15:0] f_change_state,
	input         f_change_grant,

	/* HPS interface */
	output        hps_tick_irq,
	output [47:0] hps_next_process,
	input         hps_read,

	input  [47:0] hps_change_data,

	input         hps_req,
	input         hps_address /* change (1) or sched req (0) */
);

logic  [7:0] change_type;
logic [15:0] change_pid;
logic  [7:0] change_pri;
logic [15:0] change_state;
logic [47:0] next_process;

/* convert to sched and change requests */
logic        hps_sched_req;
logic        hps_change_req;

logic        sched_req; 
logic        tick_grant;
logic        change_req;

logic        tick_irq;

assign change_type  = hps_change_data [7:0];
assign change_pid   = hps_change_data [23:8];
assign change_pri   = hps_change_data [31:24];
assign change_state = hps_change_data [47:32];
assign hps_next_process = next_process;

assign hps_sched_req  = hps_req & !hps_address;
assign hps_change_req = hps_req & hps_address; 

assign f_sched_req  = sched_req;
assign f_tick_grant = tick_grant;
assign f_change_req = change_req;

assign hps_tick_irq = tick_irq;

always_ff @(posedge clk) begin
	next_process[15:0]  <= f_next_process;
	next_process[47:16] <= 32'b0;
end


/* interrupt handling */
always_ff @(posedge clk) begin
	if (rst) begin
	end else begin
		if (f_tick_req && !f_tick_grant) begin
			tick_grant <= 1'b1;
			tick_irq <= 1'b1;
		end else if (f_tick_req && f_tick_grant) begin
		end else if (!f_tick_req && f_tick_grant) begin
			tick_grant <= 1'b0;
		end else if (!f_tick_req && !f_tick_grant && hps_read) begin
			tick_irq <= 1'b0;
		end
	end
end

/* sched_req handling */
always_ff @(posedge clk) begin
	if (rst) begin
	end else begin
		if (hps_sched_req && !f_sched_req && !f_sched_grant) begin
			sched_req <= 1'b1;
		end else if (f_sched_req && !f_sched_grant) begin
		end else if (f_sched_req && f_sched_grant) begin
			sched_req <= 1'b0;
		end else if (!f_sched_req && f_sched_grant) begin
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
		if (hps_change_req && !f_change_req && !f_change_grant) begin
			change_req <= 1'b1;
		end else if (f_change_req && !f_change_grant) begin
		end else if (f_change_req && f_change_grant) begin
			change_req <= 1'b0;
		end else if (!f_change_req && f_change_grant) begin
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
	.sched_req(f_sched_req),
	.tick_grant(f_tick_grant),
	.change_req(f_change_req),
	.change_type(f_change_type),
	.change_pid(f_change_pid),
	.change_pri(f_change_pri),
	.change_state(f_change_state),
	.sched_grant(f_sched_grant),
	.next_process(f_next_process),
	.tick_req(f_tick_req),
	.change_grant(f_change_grant)
);

endmodule

