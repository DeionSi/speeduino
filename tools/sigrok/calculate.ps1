# Assemble array of code segments for storing results
$segmentNamesInterrupts = @("TRIGPRI","TRIGSEC","TRIGTER","ONEMS")
$segmentNamesNonInterrupts = @("INIT","LOOP_start","LOOP_looptimers","LOOP_idlefueladvance","LOOP_mainloop_fuelcalcs","LOOP_mainloop_injectiontiming","LOOP_mainloop_igncalcs","LOOP_mainloop_fuelschedules","LOOP_mainloop_ignschedules","LOOP_mainloop_other","PS_pwfunction","PS_correctionsFuel","PS_docrankspeedcalcs")
$signalToSegment = $segmentNamesInterrupts + $segmentNamesNonInterrupts;

# Read input data
$sigrok_output = Get-Content sigrok_output.txt
[System.Collections.ArrayList]$framesignals = @()
$sigrok_output -replace '^(\d+)-\d+ parallel-\d+: (.*)$','$1,$2' | % { $split = $_ -split ','; $framesignals += [pscustomobject] @{frame = [int32] $split[0]; signal=[Int16] ("0x"+$split[1])} }

# Remove all single signals and all double interrupt signals, we need to make sure we start with the opening signal of a non interrupt segment
while ($framesignals.count -gt 0) {
    if ($framesignals[0].signal -eq $framesignals[1].signal -and $framesignals[0].signal -ge $segmentNamesInterrupts.Count) { break }
    else { $framesignals.RemoveAt(0) }
}

# For total duration percentage calcalations
$startFrame = $framesignals[0].Frame
$endFrame = 0

# cb = code block
$cbCurrent = -1
[System.Collections.ArrayList]$cb = @()

# Output table
[System.Collections.ArrayList]$segments = @()

foreach($fs in $framesignals)
{
    if ($fs.signal -ge $signalToSegment.Count) { Write-Error "Bad input data"; exit }
    $segmentName = $signalToSegment[$fs.signal]
    

    if ( $cbCurrent -gt -1 -and $segmentName -eq $cb[$cbCurrent].name) { # We are ending an open codeblock
        
        # Duration of this codeblock
        $cbDuration = $fs.Frame - $cb[$cbCurrent].startFrame

        # Add to the segment connected to this codeblock
        $cb[$cbCurrent].segment.count += 1
        $cb[$cbCurrent].segment.duration += $cbDuration
        $cb[$cbCurrent].segment.childrenduration += $cb[$cbCurrent].childrenTime

        # If this codeblock has a parent, add to it
        if ($cb[$cbCurrent].segment.parent -ne $null) {
            $cb[$cbCurrent-1].childrenTime += $cbDuration
        }

        # Remove this handled codeblock
        $cb.RemoveAt($cbCurrent)
        $cbCurrent--

        # Capture the frame (time) of the last closed segment for total duration calculation
        $endFrame = $fs.Frame
    }
    else { # We are in a new code block
        
        # Add the new code block
        $cb += [pscustomobject] @{ name=$segmentName; startFrame=$fs.Frame; childrenTime=0; segment=$null }
        $cbCurrent++;

        # Find in which list to check-or-add the code block
        [System.Collections.ArrayList]$segmentList = @()
        $parent = $null
        if ($fs.Signal -lt $segmentNamesInterrupts.Count -or $cbCurrent -eq 0) { # Interrupt or "root" segment
            $segmentList = $segments
        }
        else { # All "non root" segments
            $parent = $cb[$cbCurrent-1].segment
            $segmentList = $parent.children
        }

        # Make sure segment is in the list
        if ( $segmentList.name -notcontains $segmentName ) {
            [void]$segmentList.Add( [pscustomobject] @{ name=$segmentName; duration=0; childrenduration=0; children = [System.Collections.ArrayList] @(); parent=$parent; count=0 } )
        }

        # Store a reference to the target segment
        $cb[$cbCurrent].segment = $segmentList | Where-Object Name -eq $segmentName

    }

}


# Prepare the result
$totalDuration = $endFrame - $startFrame
$totalDurationInProfiling = 0

[System.Collections.ArrayList]$outputList = @()

function parseList {
    param ( $segmentsIn, [int16]$hierarchyLevel )

    $hierarchyLevel++

    $segmentsIn = $segmentsIn | select-object name,duration,@{Name = 'PercentageDecimalTotal';Expression={($_.duration/$totalTime)}},childrenduration,@{Name = 'PercentageDecimalExclChildren';Expression={(($_.duration-$_.childrenduration)/$totalTime)}},children,count

    $segmentsIn = $segmentsIn | Sort-Object -Property PercentageDecimalTotal -Descending

    Foreach ($segmentIn in $segmentsIn) {
        $Global:totalDurationInProfiling += $segmentIn.duration - $segmentIn.childrenduration
        $output = $segmentIn
        $output = $output | Select-object -Property *,@{Name = 'PercentageTotal';Expression={($_.PercentageDecimalTotal.ToString("P"))}}
        $output = $output | Select-object -Property *,@{Name = 'PercentageExclChildren';Expression={($_.PercentageDecimalExclChildren.ToString("P"))}}
        #$output = $output | Select-object -Property name,PercentageTotal,PercentageExclChildren,count
        [void]$outputList.Add( $output )

        $segmentIn.children | ForEach { $_.name = " " * 2 * $hierarchyLevel + $_.name }

        parseList -segments $segmentIn.children -hierarchyLevel $hierarchyLevel
    }
}

parseList -segments $segments -hierarchyLevel 0

$outputList | Format-Table

$totalDurationInProfiling
$totalDuration