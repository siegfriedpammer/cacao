#!/bin/nawk -f

# AWK script for generating a reducer for iburg
# this is a modified version of the BFE script

/^%%$/ {
  part+=1;
  FS="#"; 
  print $0; 
  rule=1;
  next;
}
part==0
part==1 && NF>0 {
  printf "%s = %d",$1,rule;
  if ($2!="")
    printf " (%s)",$2;
  printf ";\n";
  action1[rule]=$3;
  action2[rule]=$4;
  rule++;
  next;
}
part==2
END {
  if (part==1)
    print"%%"
  print"void burm_reduce(NODEPTR_TYPE bnode, int goalnt)";
  print"{";
  print"  int ruleNo = burm_rule (STATE_LABEL(bnode), goalnt);";
  print"  short *nts = burm_nts[ruleNo];";
  print"  NODEPTR_TYPE kids[100];";
  print"  int i;";
  print"";
  print"#if DEBUG";
  print"  fprintf (stderr, \"%s %d\\n\", burm_opname[bnode->op], ruleNo);  /* display rule */";
  print"  fflush(stderr);";
  print"#endif";
  print"  if (ruleNo==0) {";
  print"    fprintf(stderr, \"Tree cannot be derived from start symbol\\n\");";
  print"    exit(1);";
  print"  }";
  print"  switch (ruleNo) {";
  for (i in action1) {
    print "  case "i":";
    print action1[i];
    print "    break;";
  }
  print"  default:    assert (0);";
  print"  }";
  print"  burm_kids (bnode, ruleNo, kids);";
  print"  for (i = 0; nts[i]; i++)";
  print"    burm_reduce (kids[i], nts[i]);    /* reduce kids */";
  print"";
  print"";
  print"  switch (ruleNo) {";
  for (i in action2) {
    print "  case "i":";
    print action2[i];
    print "    break;";
  }
  print"  default:    assert (0);";
  print"  }";
  print"}";
}
