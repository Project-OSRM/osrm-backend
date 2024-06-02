pub fn scenario_id(scenario: &cucumber::gherkin::Scenario) -> String {
    // ports the following logic:
    // let name = scenario.getName().toLowerCase().replace(/[/\-'=,():*#]/g, '')
    // .replace(/\s/g, '_').replace(/__/g, '_').replace(/\.\./g, '.')
    // .substring(0, 64);
    let mut s = scenario
        .name
        .to_ascii_lowercase()
        .replace(
            &['/', '\\', '-', '\'', '=', ',', '(', ')', ':', '*', '#'][..],
            "",
        )
        .chars()
        .map(|x| match x {
            ' ' => '_',
            _ => x,
        })
        .collect::<String>()
        .replace('\\', "_")
        .replace("__", "_")
        .replace("..", ".");
    s.truncate(64);
    format!("{}_{}", scenario.position.line, s)
}
