module file_system (
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
	input         hps_sched_req,
	output [15:0] hps_next_process,

	input  [47:0] hps_change_data,
	input         hps_change_req
);

wire  [7:0] change_type;
wire [15:0] change_pid;
wire  [7:0] change_pri;
wire [15:0] change_state;
wire [15:0] next_process;

wire        sched_req; 
wire        tick_grant;
wire        change_req;

wire        tick_irq;

assign change_type  = hps_change_data [7:0];
assign change_pid   = hps_change_data [23:8];
assign change_pri   = hps_change_data [31:24];
assign change_state = hps_change_data [47:32];
assign hps_next_process = next_process;

assign f_sched_req  = sched_req;
assign f_tick_grant = tick_grant;
assign f_change_req = change_req;

assign hps_tick_irq = tick_irq;

always_ff @(posedge clk) begin
	next_process = f_next_process;
end

/* interrupt handling */
always_ff @(posedge clk) begin
	if (rst) begin
	end else begin
		if (f_tick_req && !f_tick_grant) begin
			f_tick_grant <= 1'b1;
			hps_tick_irq <= 1'b1;
		end else if (f_tick_req && f_tick_grant) begin
		end else if (!f_tick_req && f_tick_grant) begin
			f_tick_grant <= 1'b0;
			hps_tick_irq <= 1'b0;
		end else if (!f_tick_req && !f_tick_grant) begin
		end
	end
end

/* sched_req handling */
always_ff @(posedge clk) begin
	if (rst) begin
	end else begin
		if (hps_sched_req && !f_sched_req && !f_sched_grant) begin
			f_sched_req <= 1'b1;
		end else if (f_sched_req && !f_sched_grant) begin
		end else if (f_sched_req && f_sched_grant) begin
			f_sched_req <= 1'b0;
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
			f_change_req <= 1'b1;
		end else if (f_change_req && !f_change_grant) begin
		end else if (f_change_req && f_change_grant) begin
			f_change_req <= 1'b0;
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

