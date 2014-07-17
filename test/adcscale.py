f = open('/cms/skaplan/PLT/digi/CMSSW_5_3_9/src/DeansCode/DHidasPLT/digioutput.txt','r')
out = open('/cms/skaplan/PLT/digi/CMSSW_5_3_9/src/DeansCode/DHidasPLT/bintest.txt','w')
for line in f:
	s = line.split()
	outstring=''
	for i in s:
		if (i == s[-2]):
			adc = float(s[-2])
			newadc = int( adc*250./70000 )
			if (newadc>=250):
				newadc=249
			out.write(str(newadc)+" ")
		elif (i == s[-1]):
			out.write(i+"\n")
		else:
			out.write(i+" ")
f.close()
out.close()