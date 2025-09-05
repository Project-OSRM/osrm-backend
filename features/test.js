const message = 'hello';
const count = 42;
const greeting = `Welcome ${message}!`;
const numbers = [1, 2, 3];
const doubled = numbers.map((n) => {
  return n * 2;
});

// Export to avoid unused variable errors
export { message, count, greeting, doubled };
