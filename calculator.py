# Simple Calculator

# take operator input
op = input("Enter an operator (+, -, *, /): ")

# take two numbers
num1 = float(input("Enter first number: "))
num2 = float(input("Enter second number: "))

# perform calculation
if op == '+':
    result = num1 + num2
elif op == '-':
    result = num1 - num2
elif op == '*':
    result = num1 * num2
elif op == '/':
    if num2 != 0:
        result = num1 / num2
    else:
        print("Error! Division by zero.")
        exit()
else:
    print("Invalid operator!")
    exit()

# print result
print("Result:", result)
