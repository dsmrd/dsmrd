
function pad(num, size, decim) {
	var s = "000000" + Number(num).toFixed(decim);
	return s.substr(s.length - size);
}

$(document).ready(function() {
		setInterval(function() {

		$.getJSON('api/electricity/equipment', function(result){
			$('#equipment').html(result);
			}).fail(function() { $('#equipment').html('N/A'); });
		$.getJSON('api/gas/equipment', function(result){
			$('#device1_equipment').html(result);
			}).fail(function() { $('#device1_equipment').html('N/A'); });
		$.getJSON('api/gas/delivered', function(result){
			$('#device1_delivered').html(pad(result, 9, 3));
			}).fail(function() { $('#device1_delivered').html('N/A'); });

		$.getJSON('api/electricity/power/received', function(result){
			$('#total_recv_power').show().html(pad(result, 9, 3));
			}).fail(function() { $('#total_recv_power').html('N/A'); });
		$.getJSON('api/electricity/power/delivered', function(result){
			$('#total_power').show().html(pad(result, 9, 3));
			}).fail(function() { $('#total_power').html('N/A'); });

		$.getJSON('api/electricity/tariffs/indicator', function(result){
			if (result == 0) {
				$('tr#m1').addClass('active');
				$('tr#m2').removeClass('active');
			} else {
				$('tr#m1').removeClass('active');
				$('tr#m2').addClass('active');
			}
			});

		$.getJSON('api/electricity/phases/1/power_received', function(result){
			$('#Pn1').show().html(pad(result, 9, 3));
			}).fail(function() { $('#Pn1').html('N/A'); });
		$.getJSON('api/electricity/phases/2/power_received', function(result){
			$('#Pn2').show().html(pad(result, 9, 3));
			}).fail(function() { $('#Pn2').html('N/A'); });
		$.getJSON('api/electricity/phases/3/power_received', function(result){
			$('#Pn3').html(pad(result, 9, 3));
			}).fail(function() { $('#Pn3').html('N/A'); });
		$.getJSON('api/electricity/phases/1/power_delivered', function(result){
			$('#Pp1').html(pad(result, 9, 3));
			}).fail(function() { $('#Pp1').html('N/A'); });
		$.getJSON('api/electricity/phases/2/power_delivered', function(result){
			$('#Pp2').html(pad(result, 9, 3));
			}).fail(function() { $('#Pp2').html('N/A'); });
		$.getJSON('api/electricity/phases/3/power_delivered', function(result){
			$('#Pp3').html(pad(result, 9, 3));
			}).fail(function() { $('#Pp3').html('N/A'); });
		$.getJSON('api/electricity/phases/1/current', function(result){
			$('#A1').html(pad(result, 3, 0));
			}).fail(function() { $('#A1').html('N/A'); });
		$.getJSON('api/electricity/phases/2/current', function(result){
			$('#A2').html(pad(result, 3, 0));
			}).fail(function() { $('#A2').html('N/A'); });
		$.getJSON('api/electricity/phases/3/current', function(result){
			$('#A3').html(pad(result, 3, 0));
			}).fail(function() { $('#A3').html('N/A'); });
		$.getJSON('api/electricity/phases/1/voltage', function(result){
			$('#V1').html(pad(result, 5, 1));
			}).fail(function() { $('#V1').html('N/A'); });
		$.getJSON('api/electricity/phases/2/voltage', function(result){
			$('#V2').html(pad(result, 5, 1));
			}).fail(function() { $('#V2').html('N/A'); });
		$.getJSON('api/electricity/phases/3/voltage', function(result){
			$('#V3').html(pad(result, 5, 1));
			}).fail(function() { $('#V3').html('N/A'); });

		$.getJSON('api/electricity/tariffs/1/delivered', function(result){
			$('#delv1').html(pad(result, 10, 3));
			}).fail(function() { $('#delv1').html('N/A'); });
		$.getJSON('api/electricity/tariffs/2/delivered', function(result){
			$('#delv2').html(pad(result, 10, 3));
			}).fail(function() { $('#delv2').html('N/A'); });
		$.getJSON('api/electricity/tariffs/1/received', function(result){
			$('#recv1').html(pad(result, 10, 3));
			}).fail(function() { $('#recv1').html('N/A'); });
		$.getJSON('api/electricity/tariffs/2/received', function(result){
			$('#recv2').html(pad(result, 10, 3));
			}).fail(function() { $('#recv2').html('N/A'); });

		$.getJSON('api/electricity/tariff1', function(result){
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
		$.getJSON('api/electricity/tariff2', function(result){
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
}, 2000);
});

