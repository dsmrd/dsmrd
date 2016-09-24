
$(document).ready(function() {
		setInterval(function() {

			$.getJSON('api/devices/0/equipment', function(result){
				$('#equipment').html(result);
				}).fail(function() {
					$('#equipment').html('N/A');
					});

			$.getJSON('api/devices/1/type', function(result){
				$('#devices_1_type').html(result);
				}).fail(function() {
					$('#devices_1_type').html('N/A');
					});
			$.getJSON('api/devices/1/equipment', function(result){
				$('#devices_1_equipment').html(result);
				}).fail(function() {
					$('#devices_1_equipment').html('N/A');
					});
			$.getJSON('api/devices/1/timestamp', function(result){
				var d = new Date(result*1000);
				$('#devices_1_timestamp').html(d);
				}).fail(function() {
					$('#devices_1_timestamp').html('N/A');
					});
			$.getJSON('api/devices/1/phases/0/delivered', function(result){
				$('#devices_1_delivered').html(Number(result).toFixed(3));
				}).fail(function() {
					$('#devices_1_delivered').html('N/A');
					});

			$.getJSON('api/devices/2/type', function(result){
					$('#device_2_type').html(result);
					}).fail(function() {
						$('#device_2_type').html('N/A');
						});
			$.getJSON('api/devices/2/equipment', function(result){
					$('#device_2_equipment').html(result);
					}).fail(function() {
						$('#device_2_equipment').html('N/A');
						});
			$.getJSON('api/devices/2/timestamp', function(result){
				var d = new Date(result*1000);
					$('#device_2_timestamp').html(d);
					}).fail(function() {
						$('#device_2_timestamp').html('N/A');
						});
			$.getJSON('api/devices/2/phases/0/delivered', function(result){
					$('#device_2_delivered').html(Number(result).toFixed(3));
					}).fail(function() {
						$('#device_2_delivered').html('N/A');
						});

			$.getJSON('api/devices/3/type', function(result){
					$('#device_3_type').html(result);
					}).fail(function() {
						$('#device_3_type').html('N/A');
						});
			$.getJSON('api/devices/3/equipment', function(result){
					$('#device_3_equipment').html(result);
					}).fail(function() {
						$('#device_3_equipment').html('N/A');
						});
			$.getJSON('api/devices/3/timestamp', function(result){
				var d = new Date(result*1000);
					$('#device_3_timestamp').html(d);
					}).fail(function() {
						$('#device_3_timestamp').html('N/A');
						});
			$.getJSON('api/devices/3/phases/0/delivered', function(result){
					$('#device_3_delivered').html(Number(result).toFixed(3));
					}).fail(function() {
						$('#device_3_delivered').html('N/A');
						});

			$.getJSON('api/devices/4/type', function(result){
					$('#device_4_type').html(result);
					}).fail(function() {
						$('#device_4_type').html('N/A');
						});
			$.getJSON('api/devices/4/equipment', function(result){
					$('#device_4_equipment').html(result);
					}).fail(function() {
						$('#device_4_equipment').html('N/A');
						});
			$.getJSON('api/devices/4/timestamp', function(result){
				var d = new Date(result*1000);
					$('#device_4_timestamp').html(d);
					}).fail(function() {
						$('#device_4_timestamp').html('N/A');
						});
			$.getJSON('api/devices/4/phases/0/delivered', function(result){
					$('#device_4_delivered').html(Number(result).toFixed(3));
					}).fail(function() {
						$('#device_4_delivered').html('N/A');
						});

			$.getJSON('api/devices/0/power/received', function(result){
					$('#total_recv_power').show().html(Number(result).toFixed(3));
					}).fail(function() {
						$('#total_recv_power').html('N/A');
						});
			$.getJSON('api/devices/0/power/delivered', function(result){
					$('#total_power').show().html(Number(result).toFixed(3));
					}).fail(function() {
						$('#total_power').html('N/A');
						});

			$.getJSON('api/devices/0/tariffs/indicator', function(result){
					$('#indicator').html(Number(result).toFixed(3));
					}).fail(function() {
						$('#indicator').html('N/A');
						});

			$.getJSON('api/devices/0/tariffs/indicator', function(result){
					if (result == 0) {
					$('tr#m1').addClass('active');
					$('tr#m2').removeClass('active');
					} else {
					$('tr#m1').removeClass('active');
					$('tr#m2').addClass('active');
					}
					});

			$.getJSON('api/devices/0/phases/1/power_received', function(result){
					$('#Pn1').show().html(Number(result).toFixed(3));
					}).fail(function() { $('#Pn1').html('N/A'); });
			$.getJSON('api/devices/0/phases/2/power_received', function(result){
					$('#Pn2').show().html(Number(result).toFixed(3));
					}).fail(function() { $('#Pn2').html('N/A'); });
			$.getJSON('api/devices/0/phases/3/power_received', function(result){
					$('#Pn3').html(Number(result).toFixed(3));
					}).fail(function() { $('#Pn3').html('N/A'); });
			$.getJSON('api/devices/0/phases/1/power_delivered', function(result){
					$('#Pp1').html(Number(result).toFixed(3));
					}).fail(function() { $('#Pp1').html('N/A'); });
			$.getJSON('api/devices/0/phases/2/power_delivered', function(result){
					$('#Pp2').html(Number(result).toFixed(3));
					}).fail(function() { $('#Pp2').html('N/A'); });
			$.getJSON('api/devices/0/phases/3/power_delivered', function(result){
					$('#Pp3').html(Number(result).toFixed(3));
					}).fail(function() { $('#Pp3').html('N/A'); });
			$.getJSON('api/devices/0/phases/1/current', function(result){
					$('#A1').html(result);
					}).fail(function() { $('#A1').html('N/A'); });
			$.getJSON('api/devices/0/phases/2/current', function(result){
					$('#A2').html(result);
					}).fail(function() { $('#A2').html('N/A'); });
			$.getJSON('api/devices/0/phases/3/current', function(result){
					$('#A3').html(result);
					}).fail(function() { $('#A3').html('N/A'); });
			$.getJSON('api/devices/0/phases/1/voltage', function(result){
					$('#V1').html(Number(result).toFixed(1));
					}).fail(function() { $('#V1').html('N/A'); });
			$.getJSON('api/devices/0/phases/2/voltage', function(result){
					$('#V2').html(Number(result).toFixed(1));
					}).fail(function() { $('#V2').html('N/A'); });
			$.getJSON('api/devices/0/phases/3/voltage', function(result){
					$('#V3').html(Number(result).toFixed(1));
					}).fail(function() { $('#V3').html('N/A'); });

			$.getJSON('api/devices/0/tariffs/1/delivered', function(result){
					$('#delv1').html(Number(result).toFixed(3));
					}).fail(function() { $('#delv1').html('N/A'); });
			$.getJSON('api/devices/0/tariffs/2/delivered', function(result){
					$('#delv2').html(Number(result).toFixed(3));
					}).fail(function() { $('#delv2').html('N/A'); });
			$.getJSON('api/devices/0/tariffs/1/received', function(result){
					$('#recv1').html(Number(result).toFixed(3));
					}).fail(function() { $('#recv1').html('N/A'); });
			$.getJSON('api/devices/0/tariffs/2/received', function(result){
					$('#recv2').html(Number(result).toFixed(3));
					}).fail(function() { $('#recv2').html('N/A'); });

			$.getJSON('api/version', function(result){
					$('#version').html(result);
					}).fail(function() { $('#version').html('N/A'); });

			$.getJSON('api/devices/0/timestamp', function(result){
					var d = new Date(result*1000);
					$('#datetimestamp').html(d);
					}).fail(function() { $('#datetimestamp').html('N/A'); });

			$.getJSON('api/devices/0/nof_power_failures', function(result){
					$('#nof_power_failures').html(result);
					}).fail(function() { $('#nof_power_failures').html('N/A'); });
			$.getJSON('api/devices/0/nof_long_power_failures', function(result){
					$('#nof_long_power_failures').html(result);
					}).fail(function() { $('#nof_long_power_failures').html('N/A'); });
			$.getJSON('api/devices/0/power_fail_event_log', function(result){
					$('#power_fail_event_log').html(result);
					}).fail(function() { $('#power_fail_event_log').html('N/A'); });
			$.getJSON('api/devices/0/phases/1/nof_voltage_sage', function(result){
					$('#nof_voltage_sage_l1').html(result);
					}).fail(function() { $('#nof_voltage_sage_l1').html('N/A'); });
			$.getJSON('api/devices/0/phases/2/nof_voltage_sage', function(result){
					$('#nof_voltage_sage_l2').html(result);
					}).fail(function() { $('#nof_voltage_sage_l2').html('N/A'); });
			$.getJSON('api/devices/0/phases/3/nof_voltage_sage', function(result){
					$('#nof_voltage_sage_l3').html(result);
					}).fail(function() { $('#nof_voltage_sage_l3').html('N/A'); });
			$.getJSON('api/devices/0/phases/1/nof_voltage_swells', function(result){
					$('#nof_voltage_swells_l1').html(result);
					}).fail(function() { $('#nof_voltage_swells_l1').html('N/A'); });
			$.getJSON('api/devices/0/phases/2/nof_voltage_swells', function(result){
					$('#nof_voltage_swells_l2').html(result);
					}).fail(function() { $('#nof_voltage_swells_l2').html('N/A'); });
			$.getJSON('api/devices/0/phases/3/nof_voltage_swells', function(result){
					$('#nof_voltage_swells_l3').html(result);
					}).fail(function() { $('#nof_voltage_swells_l3').html('N/A'); });
			$.getJSON('api/devices/0/message/0', function(result){
					$('#text_message0').html(result);
					}).fail(function() { $('#text_message0').html('N/A'); });
			$.getJSON('api/devices/0/message/1', function(result){
					$('#text_message1').html(result);
					}).fail(function() { $('#text_message1').html('N/A'); });

/*
			$.getJSON('api/devices/0/tariffs/1/delivered', function(result){
					var value = result.value;
					$('#t10').html(Math.floor((value/100000) % 10));
					$('#t11').html(Math.floor((value/10000) % 10));
					$('#t12').html(Math.floor((value/1000) % 10));
					$('#t13').html(Math.floor((value/100) % 10));
					$('#t14').html(Math.floor((value/10) % 10));
					$('#t15').html(Math.floor(value % 10));
					$('#t16').html(Math.floor((value*10) % 10));
					$('#t17').html(Math.floor((value*100) % 10));
					$('#t18').html(Math.floor((value*1000) % 10));
					}).fail(function() {
						$('.nonfract').html('-');
						$('.fract').html('-');
						});
			$.getJSON('api/devices/0/tariffs/2/delivered', function(result){
					var value = result.value;
					$('#t20').html(Math.floor((value/100000) % 10));
					$('#t21').html(Math.floor((value/10000) % 10));
					$('#t22').html(Math.floor((value/1000) % 10));
					$('#t23').html(Math.floor((value/100) % 10));
					$('#t24').html(Math.floor((value/10) % 10));
					$('#t25').html(Math.floor(value % 10));
					$('#t26').html(Math.floor((value*10) % 10));
					$('#t27').html(Math.floor((value*100) % 10));
					$('#t28').html(Math.floor((value*1000) % 10));
					}).fail(function() {
						$('.nonfract').html('-');
						$('.fract').html('-');
						});
*/

		}, 2000);
});

