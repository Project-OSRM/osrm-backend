@routing @bicycle @roundabout @instruction
Feature: Roundabout Instructions
	
	Background:
		Given the profile "bicycle"
	
	Scenario: Bicycle - Roundabout instructions
		Given the node map
		 |   |   | t |   |   |
		 |   |   | b |   |   |
		 | s | a |   | c | u |
		 |   |   | d |   |   |
		 |   |   | v |   |   |

		And the ways
		 | nodes | junction   |
		 | sa    |            |
		 | tb    |            |
		 | uc    |            |
		 | vd    |            |
		 | abcda | roundabout |
 
		When I route I should get
		 | from | to | route | turns                             |
		 | s    | v  | sa,vd | head,enter_roundabout,destination |
		 | s    | u  | sa,uc | head,enter_roundabout,destination |
		 | s    | t  | sa,tb | head,enter_roundabout,destination |
		 | u    | t  | uc,tb | head,enter_roundabout,destination |
		 | u    | s  | uc,sa | head,enter_roundabout,destination |
		 | u    | v  | uc,vd | head,enter_roundabout,destination |
