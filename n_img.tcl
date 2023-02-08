##############################################################################
proc n_img_empty_string {s} {
	if {![string compare $s "empty"]} {
        return ""
    } else {
        return $s
    }
}

##############################################################################
proc n_img_this_integer {s default} {
	if {![string is integer -strict $s]} {
        return $default
    } else {
        return [string trim $s]
    }
}	

##############################################################################
proc n_img_this_num {s default} {
	if {![string is double $s]} {
        return $default
    } else {
        return [string trim $s]
    }
}	

##############################################################################
proc n_img_this_string {s} {
	set buf [string map {" " _} [string trim $s]]
	set buf [string map {\$ \\$} $buf]
	if {[string length $s] == "0"} {
        return "empty"
    } else {
        return $buf
    }
}	

##############################################################################
proc n_img_apply {id} {
	set vid             [string trimleft $id .]
	set v_filename      [concat v_filename_$vid]
	set v_frames        [concat v_frames_$vid]
	set v_pos           [concat v_pos_$vid]

	global $v_filename      
	global $v_frames        
	global $v_pos

	set b_filename       [n_img_this_string  [set $v_filename]]
	set $v_frames        [n_img_this_integer [set $v_frames] 1]
	set $v_pos           [n_img_this_integer [set $v_pos] 0]

	set cmd [concat $id dialog \
                 $b_filename \
                 [eval concat $$v_frames] \
                 [eval concat $$v_pos] \
                 \;]
    
	pdsend $cmd
}

##############################################################################
proc n_img_cancel {id} {
	set cmd [concat $id cancel \;]
	pdsend $cmd
}

##############################################################################
proc n_img_ok {id} {
	n_img_apply $id
	n_img_cancel $id
}

##############################################################################
proc pdtk_n_img_dialog {id filename frames pos } {
	set vid             [string trimleft $id .]
	set v_filename      [concat v_filename_$vid]
	set v_frames        [concat v_frames_$vid]
	set v_pos           [concat v_pos_$vid]

	global $v_filename      
	global $v_frames        
	global $v_pos

	set $v_filename      $filename      
	set $v_frames        $frames        
	set $v_pos           $pos

	##########################################################################
	# setup
	toplevel $id
	wm title $id {n_img}
	wm protocol $id WM_DELETE_WINDOW [concat n_img_cancel $id]
	label $id.lab -text {EDITOR PROPERTIES}
	pack $id.lab -side top

	##########################################################################
	# cancel apply ok
	frame $id.bf
	pack $id.bf -side bottom -fill x -pady 2m
	button $id.bf.cancel -text {Cancel} -width 10 -command "n_img_cancel $id"
	button $id.bf.apply  -text {Apply}  -width 10 -command "n_img_apply $id"
	button $id.bf.ok     -text {Ok}     -width 10 -command "n_img_ok $id"
	pack $id.bf.cancel -side left -expand 1
	pack $id.bf.apply  -side left -expand 1
	pack $id.bf.ok     -side left -expand 1

	##########################################################################
	# param
	#
	frame $id.f_filename
	pack  $id.f_filename -side top
	label $id.f_filename.l -text "      filename:"
	entry $id.f_filename.e -textvariable $v_filename -width 22
	pack  $id.f_filename.l $id.f_filename.e -side left

	#
	frame $id.f_frames
	pack  $id.f_frames -side top
	label $id.f_frames.l -text "        frames:"
	entry $id.f_frames.e -textvariable $v_frames -width 22
	pack  $id.f_frames.l $id.f_frames.e -side left
    
	#
	frame $id.f_pos
	pack  $id.f_pos -side top
	label $id.f_pos.l -text "        pos:"
	entry $id.f_pos.e -textvariable $v_pos -width 22
	pack  $id.f_pos.l $id.f_pos.e -side left
    

	##########################################################################
	# bind focus
	bind $id <KeyPress-Return> [concat n_img_ok $id]
	bind $id <Control-w> [concat n_img_cancel $id]
	focus $id.f_filename.e
}
