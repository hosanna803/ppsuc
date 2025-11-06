# include <stdio.h>
 void main()
 {
 	 int h,rev=0 ,rem,tem;
 	 		printf("enter number:");
 	  scanf("%d",&h);
 	  tem=h;
 	  while(h!=0)
 {
 rem=h%10;
  rev=rev*10+rem;
   h=h/10;
}
 if (rev==tem)
 {printf("%d is palidrome",tem);
 
 }
 else{ printf("%d is not palidrome");
 }
 return 0;
}
