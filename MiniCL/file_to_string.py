#!/usr/bin/python
# Converts the contents of a file to a C string constant
# EB Mar 2011

# Options:
# -i INPUT_FILENAME
# -o OUTPUT_FILENAME
# -s VARIABLE_NAME

import getopt,sys

def usage():
  print "Usage: -i input_filename -o output_filename -s variable_name"

def main():
  # Parse command line options
  try:
    opts,args = getopt.getopt(sys.argv[1:],"i:o:s:")
  except getopt.GetoptError, err:
    print str(err)
    usage()
    sys.exit(2)
  i_file = None
  o_file = None
  s_name = None
  for opt,val in opts:
    if opt == '-i':
      i_file = val
    elif opt == '-o':
      o_file = val
    elif opt == '-s':
      s_name = val
    else:
      assert False, opt
  print "Generating OpenCL code string %s:"%s_name
  print "< %s"%i_file
  print "> %s"%o_file
  f = open(i_file,'r')
  g = open(o_file,'w')
  g.write("const char * %s =\n"%s_name)
  for line in f:
    line = line.replace("\n","")
    line = line.replace("\r","")
    line = line.replace("\\","\\\\")
    g.write("\"%s\\n\"\n"%line)
  g.write(";\n")
  f.close()
  g.close()
  sys.exit(0)

if __name__ == "__main__":
  main()
