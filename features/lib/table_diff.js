// Table comparison utility for displaying colorized differences between expected and actual test results
import chalk from 'chalk';

const unescapeStr = (str) => str.replace(/\\\|/g, '|').replace(/\\\\/g, '\\');

String.prototype.padLeft = function (char, length) {
  return char.repeat(Math.max(0, length - this.length)) + this;
};

String.prototype.padRight = function (char, length) {
  return this + char.repeat(Math.max(0, length - this.length));
};

export default function (expected, actual) {
  const headers = expected.raw()[0];
  const expectedRows = expected.hashes();
  let tableError = false;
  const statusRows = [];
  const columnStatus = {};

  expectedRows.forEach((expectedRow, i) => {
    statusRows[i] = {};
    const statusRow = statusRows[i];
    for (const key in expectedRow) {
      const actualRow = actual[i];
      if (unescapeStr(expectedRow[key]) != actualRow[key]) {
        statusRow[key] = false;
        tableError = true;
        columnStatus[key] = false;
      }
    }
  });

  if (!tableError) return null;

  // determine column widths
  const widths = {};
  const wantStr = '(-) ';
  const gotStr = '(+) ';
  const okStr = '    ';

  headers.forEach((key) => {
    widths[key] = key.length;
  });

  expectedRows.forEach((row, _i) => {
    headers.forEach((key) => {
      const content = row[key];
      const length = content.length;
      if (widths[key] == null || length > widths[key]) widths[key] = length;
    });
  });

  // format
  const lines = [chalk.red('Tables were not identical:')];
  let cells = [];

  // header row
  cells = [];
  headers.forEach((key) => {
    let content = String(key).padRight(' ', widths[key]);
    if (columnStatus[key] == false) content = okStr + content;
    cells.push(chalk.white(content));
  });
  lines.push(`| ${cells.join(' | ')} |`);

  // content rows
  expectedRows.forEach((row, i) => {
    let cells;
    const rowError = Object.keys(statusRows[i]).length > 0;

    // expected row
    cells = [];
    headers.forEach((key) => {
      let content = String(row[key]).padRight(' ', widths[key]);
      if (statusRows[i][key] == false)
        cells.push(chalk.yellow(wantStr + content));
      else {
        if (rowError) {
          if (columnStatus[key] == false) content = okStr + content;
          cells.push(chalk.yellow(content));
        } else {
          if (columnStatus[key] == false) content = okStr + content;
          cells.push(chalk.green(content));
        }
      }
    });
    lines.push(`| ${cells.join(' | ')} |`);

    // if error in row, insert extra row showing actual result
    if (rowError) {
      cells = [];
      headers.forEach((key) => {
        const content = String(actual[i][key]).padRight(' ', widths[key]);
        if (statusRows[i][key] == false)
          cells.push(chalk.red(gotStr + content));
        else {
          if (columnStatus[key] == false)
            cells.push(chalk.red(okStr + content));
          else cells.push(chalk.red(content));
        }
      });

      lines.push(`| ${cells.join(' | ')} |`);
    }
  });
  return lines.join('\n');
}
