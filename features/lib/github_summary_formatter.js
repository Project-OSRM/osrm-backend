import fs from 'node:fs';
import { SummaryFormatter } from '@cucumber/cucumber';

const FAILURES = ['FAILED', 'AMBIGUOUS', 'UNDEFINED'];
const WARNINGS = ['SKIPPED', 'PENDING', 'UNKNOWN'];
// const SUCCESSES = ['PASSED'];

function isFailure(result) {
  return FAILURES.includes(result.status);
}
function isWarning(result) {
  return WARNINGS.includes(result.status);
}
// function isSuccess(result) {
//   return SUCCESSES.includes(result.status);
// }
function toSeconds(timeStamp) {
  return +timeStamp.seconds + timeStamp.nanos / 1e9;
}

/**
 * A Summary Formatter that also appends to $GITHUB_STEP_SUMMARY
 *
 * This formatter first calls the stock summary formatter, then appends one line of
 * MarkDown to the file denoted by $GITHUB_STEP_SUMMARY.
 *
 * The hacky part is that we have to *append* to $GITHUB_STEP_SUMMARY. By using the
 * obvious:
 *
 *   --format github_summary_formatter:"$GITHUB_STEP_SUMMARY"
 *
 * we would *overwrite* that file.
 *
 * Another thing to consider is that Cucumber only allows one formatter that grabs
 * stdout.  In order to avoid the clumsy:
 *
 *   --format summary --format github_summary_formatter:"/dev/null"
 *
 * this formatter also does the duty of the stock summary formatter:
 *
 *   --format github_summary_formatter
 *
 * Override the default filename of: $GITHUB_STEP_SUMMARY
 *
 *   formatOptions: { 'github_summary_formatter': { filename: 'somewhere.else' }},
 */

export default class GithubSummaryFormatter extends SummaryFormatter {
  static documentation = 'A Summary Formatter that also appends a $GITHUB_STEP_SUMMARY';
  constructor(options) {
    super(options);
    this.filename = options.parsedArgvOptions?.github_summary_formatter?.filename
        ?? process.env.GITHUB_STEP_SUMMARY;
  }

  logSummary(testRunDuration) {
    // because Cucumber only allows one formatter thats grab stdout,
    // we have to mimic the summary formatter first
    super.logSummary(testRunDuration);
    if (!this.filename)
      return;

    let failures = 0;
    let warnings = 0;
    let successes = 0;

    for (const { worstTestStepResult } of this.eventDataCollector.getTestCaseAttempts()) {
      if (isFailure(worstTestStepResult)) {
        ++failures;
      } else if (isWarning(worstTestStepResult)) {
        ++warnings;
      } else {
        ++successes;
      }
    };
    if (failures > 0) {
      failures = `:x: ${failures}`;
    }
    const seconds = toSeconds(testRunDuration).toFixed(3);
    const msg = `| ${successes} | ${warnings} | ${failures} | ${seconds} |\n`;

    fs.appendFileSync(this.filename, msg);
  }
}
