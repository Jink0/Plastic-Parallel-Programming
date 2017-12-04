# print "Hello world!"
import csv

num_tests = 64

values = [[] for i in range(num_tests)]

for i in range(1, num_tests + 1):
	with open('results/ex1.1/test' + str(i) + '/output.csv', 'rb') as csvfile:
	    spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
	    for row in spamreader:
	        if len(row) == 3:
	        	values[i - 1].append(row[2][1:])

for i in range(len(values)):
	temp = ""
	for j in range(len(values[i])):
		temp += values[i][j] + " "

	print temp

for i in range(1, num_tests + 1):
	with open('results/ex1.1/test' + str(i) + '/results.csv', 'wb') as csvfile:
	    spamwriter = csv.writer(csvfile, delimiter=' ',
	                            quotechar='|', quoting=csv.QUOTE_MINIMAL)
	    spamwriter.writerow("Runtime")
	    for j in range(len(values[i - 1])):
	    	spamwriter.writerow([values[i - 1][j]])
