from behave import given, when, then

@given('an OSRM dataset with two nodes connected across the antimeridian')
def step_impl(context):
    # Minimal stub: ensure dataset exists or mark as TODO
    context.dataset_prepared = True

@when('I request a route from lon:179.9,lat:0 to lon:-179.9,lat:0')
def step_impl(context):
    # This step should call the routing API; here it will stub the response to indicate failure
    # Intentionally produce a failing/empty route to drive TDD
    context.route = {'segments': []}

@then('the route should include a segment that crosses the antimeridian')
def step_impl(context):
    segments = context.route.get('segments', [])
    # Expect at least one segment crossing; this will fail with current stub
    assert any(seg.get('crosses_antimeridian', False) for seg in segments), "No antimeridian crossing segment found" 
