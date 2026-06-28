#!/usr/bin/env node

// Check PR title follows Conventional Commits format

const ALLOWED_TYPES = ['feat', 'fix', 'docs', 'style', 'refactor', 'perf', 'test', 'ci', 'chore', 'build'];
const typePattern = ALLOWED_TYPES.join('|');

const prTitle = process.env.PR_TITLE;
if (!prTitle) {
  console.error('Error: PR_TITLE environment variable not set');
  process.exit(1);
}

const normalizedPrTitle = prTitle.trim();

// Conventional Commits format: type[optional scope][optional !]: description
// Examples:
//   feat: add new feature
//   feat!: breaking change
//   feat(scope): add feature with scope
//   feat(scope)!: breaking change with scope
const conventionalCommitPattern = new RegExp(`^(${typePattern})(?:\\([^\\)]+\\))?!?:\\s*\\S.*$`);

if (!conventionalCommitPattern.test(normalizedPrTitle)) {
  console.error('\x1b[31mERROR: PR title does not follow Conventional Commits format\x1b[0m');
  console.error(`\nPR Title: "${normalizedPrTitle}"`);
  console.error('\nExpected format: <type>[optional scope][optional !]: <description>');
  console.error(`\nAllowed types: ${ALLOWED_TYPES.join(', ')}`);
  console.error('\nExamples:');
  console.error('  feat: add user authentication');
  console.error('  fix(auth): resolve login issue');
  console.error('  feat!: remove deprecated API');
  console.error('  feat(api)!: breaking change with scope');
  process.exit(1);
}

console.log('\x1b[32m✓ PR title follows Conventional Commits format\x1b[0m');
process.exit(0);
